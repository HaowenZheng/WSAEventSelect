#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<iostream>
using namespace std;

#include<WinSock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#define MAX_EVENT 2048


void EventSelectSock()
{
	//����socket 
	SOCKET listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listensock == INVALID_SOCKET)
	{
		cout << "����listen socket ʧ�ܣ�" << WSAGetLastError() << endl;
		return;
	}

	//���ýṹ��
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

	//��
	auto ret = bind(listensock, (sockaddr*)&listenaddr, sizeof(sockaddr));
	if (ret != 0)
	{
		cout << "bind error" << endl;
		return;
	}

	//����
	ret = listen(listensock, 5);
	if (ret != 0)
	{
		cout << "listen error" << endl;
		return;
	}
	cout << "�ɹ������˿�: " << ntohs(listenaddr.sin_port) << endl;

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
			cout << "�ȴ�ʱ���ʹ���, �������: " << WSAGetLastError() << endl;
			break;
		}

		nindex -= WSA_WAIT_EVENT_0;

		auto sock = sockArray[nindex];
		auto hand = eventArray[nindex];
		WSANETWORKEVENTS wsaEvent = { 0 };

		WSAEnumNetworkEvents(sock, hand, &wsaEvent);

		//�ж� event ��errorcode
		if (wsaEvent.lNetworkEvents & FD_ACCEPT)
		{
			if (wsaEvent.iErrorCode[FD_ACCEPT_BIT] != 0)
			{
				cout << "accep error: " << WSAGetLastError() << endl;
				continue;
			}

			if (nindex >= MAX_EVENT)
			{
				cout << "����̫��" << endl;
				continue;
			}

			//��������
			sockaddr_in clientaddr = { 0 };
			int lens = sizeof(sockaddr_in);
			SOCKET client = accept(listensock, (sockaddr*)&clientaddr, &lens);
			if (client == INVALID_SOCKET)
			{
				cout << "��Ч��socket" << endl;
				continue;
			}
			cout << "������: " << inet_ntoa(clientaddr.sin_addr) << " : " << ntohs(clientaddr.sin_port) << endl;

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
			cout << "�Ͽ�һ������" << endl;

			//����array
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
				cout << "�յ�����: " << buf << endl;

				//���ݷŵ�����queue�У����������߳�ȥ�������ݣ����ﲻ��������ʵ���첽���������ݷ��ػ��ǻ�����

				char sendmsg[] = "Receive your message";
				int nSend = send(sock, sendmsg, strlen(sendmsg), 0);
				if (nSend != strlen(sendmsg))
				{
					cout << "��Ϣ���Ͳ���ȫ" << endl;
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