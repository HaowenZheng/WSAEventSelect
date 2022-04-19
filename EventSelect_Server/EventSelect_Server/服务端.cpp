#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<iostream>
using namespace std;

#include<WinSock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#define MAX_EVENT 2048


void EventSelectSock()
{
	//创建socket 
	SOCKET listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listensock == INVALID_SOCKET)
	{
		cout << "创建listen socket 失败：" << WSAGetLastError() << endl;
		return;
	}

	//配置结构体
	sockaddr_in listenaddr;
	memset(&listenaddr, 0, sizeof(sockaddr_in));

	listenaddr.sin_port = htons(27015);
	listenaddr.sin_family = AF_INET;
	//listenaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//listenaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	hostent* thishost = gethostbyname("");
	char* ip;
	ip = inet_ntoa(*(in_addr*)*thishost->h_addr_list);
	inet_pton(AF_INET, ip, &listenaddr.sin_addr.s_addr);

	//绑定
	auto ret = bind(listensock, (sockaddr*)&listenaddr, sizeof(sockaddr));
	if (ret != 0)
	{
		cout << "bind error" << endl;
		return;
	}

	//监听
	ret = listen(listensock, 5);
	if (ret != 0)
	{
		cout << "listen error" << endl;
		return;
	}
	cout << "成功监听端口: " << ntohs(listenaddr.sin_port) << endl;

	WSAEVENT eventArray[MAX_EVENT];
	SOCKET   sockArray[MAX_EVENT];

	int nEvent = 0;
	HANDLE event0 = WSACreateEvent();
	WSAEventSelect(listensock, event0, FD_ACCEPT | FD_CLOSE);
	eventArray[0] = event0;
	sockArray[0] = listensock;
	nEvent++;

	while (1)
	{
		int nindex = WSAWaitForMultipleEvents(nEvent, eventArray, false, WSA_INFINITE, false);

		if (nindex == WSA_WAIT_IO_COMPLETION || nindex == WSA_WAIT_TIMEOUT)
		{
			cout << "等待时发送错误, 错误代码: " << WSAGetLastError() << endl;
			break;
		}

		nindex -= WSA_WAIT_EVENT_0;

		auto sock = sockArray[nindex];
		auto hand = eventArray[nindex];
		WSANETWORKEVENTS wsaEvent = { 0 };

		WSAEnumNetworkEvents(sock, hand, &wsaEvent);

		//判断 event 和errorcode
		if (wsaEvent.lNetworkEvents & FD_ACCEPT)
		{
			if (wsaEvent.iErrorCode[FD_ACCEPT_BIT] != 0)
			{
				cout << "accep error: " << WSAGetLastError() << endl;
				continue;
			}

			if (nindex >= MAX_EVENT)
			{
				cout << "链接太多" << endl;
				continue;
			}

			//增加链接
			sockaddr_in clientaddr = { 0 };
			int lens = sizeof(sockaddr_in);
			SOCKET client = accept(listensock, (sockaddr*)&clientaddr, &lens);
			if (client == INVALID_SOCKET)
			{
				cout << "无效的socket" << endl;
				continue;
			}
			cout << "新链接: " << inet_ntoa(clientaddr.sin_addr) << " : " << ntohs(clientaddr.sin_port) << endl;

			WSAEVENT newEvent = WSACreateEvent();
			WSAEventSelect(client, newEvent, FD_WRITE | FD_READ | FD_CLOSE);

			eventArray[nEvent] = newEvent;
			sockArray[nEvent] = client;
			nEvent++;
		}
		else if (wsaEvent.lNetworkEvents & FD_CLOSE)
		{
			WSACloseEvent(eventArray[nindex]);
			closesocket(sockArray[nindex]);
			cout << "断开一个链接" << endl;

			//更新array
			for (int j = nindex; j < nEvent - 1; j++)
			{
				eventArray[j] = eventArray[j + 1];
				sockArray[j] = sockArray[j + 1];
			}
			--nEvent;
		}
		else if (wsaEvent.lNetworkEvents & FD_READ)
		{
			if (wsaEvent.iErrorCode[FD_READ_BIT] != 0)
				continue;

			char buf[1024] = { 0 };
			int nRecv = recv(sock, buf, 1024, 0);

			if (nRecv > 0)
			{
				cout << "收到数据: " << buf << endl;

				//数据放到队列queue中，其他其他线程去处理数据，这里不会阻塞，实现异步，但是数据返回还是会阻塞

				char sendmsg[] = "Receive your message";
				int nSend = send(sock, sendmsg, strlen(sendmsg), 0);
				if (nSend != strlen(sendmsg))
				{
					cout << "消息发送不完全" << endl;
				}
			}
		}
	}

}

int main()
{
	WSADATA wsaData = { 0 };
	auto ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != 0)
	{
		cout << "wsa start up error: " << WSAGetLastError() << endl;
		return -1;
	}

	EventSelectSock();

	WSACleanup();

	return 0;
}