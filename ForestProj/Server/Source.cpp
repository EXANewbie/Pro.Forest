#include <cstdio>
#include <cstdio>
#include <WinSock2.h>
#include <iostream>

using namespace std;

void main() {
	WSADATA wasData;
	SOCKET ListeningSocket;
	SOCKET NewConnection;
	SOCKADDR_IN ServerAddr;
	SOCKADDR_IN ClientAddr;
	int Port = 78911;

	// ������ ���� 2.2�� �ʱ�ȭ
	int Ret;
	if ((Ret = WSAStartup(MAKEWORD(2, 2), &wasData)) != 0) {
		printf("WASStartup failed with error %d\n", Ret);
		return;
	} // ������ ��ٸ��� ���� ���� ����

	ListeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListeningSocket == INVALID_SOCKET)
	{
		printf("error %d\n", WSAGetLastError());
		return;
	}

	// bind�ϱ� ���� SOCKADDR_IN�� ��Ʈ��ȣ Port�� INADDR_ANY�� ����
	// ����, ȣ��Ʈ ����Ʈ ������ ��Ʈ�� ����Ʈ ������ �ٲ���Ѵ�!

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(Port);
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind�� �̿��Ͽ� ����� �ּ� ����

	int tmp = bind(ListeningSocket, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr));
	/*
	if (bind(server, (sockaddr *)&sin, sizeof(sin) == SOCKET_ERROR))
	{
	wError = WSAGetLastError();
	cerr << "Error: bin() return value == SOCKET_ERROR\n"
	"Details: " << wError << endl;
	WSACleanup();
	exit(EXIT_FAILURE);
	}*/
	if (tmp == SOCKET_ERROR)
	{
		printf("tmp : %d %d %d %d\n", tmp, WSAGetLastError(), WSAEFAULT, WSAEADDRINUSE);
	}

	// Ŭ���̾�Ʈ�� ������ ��ٸ�
	// backlog�� �Ϲ������� 5

	listen(ListeningSocket, 5);

	printf("%d\n", WSAGetLastError());

	// ���ο� ������ �ϳ� ����
	int ClientAddrLen = sizeof(ClientAddr); // Ŭ���̾�Ʈ ��巹���� ���̸� ����
	NewConnection = accept(ListeningSocket, (SOCKADDR *)&ClientAddr, &ClientAddrLen);

	// ���⿡ �� ���� ���´�.
	


	cout << "������ �����Ͽ����ϴ�." << endl;

	SOCKADDR_IN temp_sock;
	int temp_sock_size = sizeof(temp_sock);

	getpeername(NewConnection, (SOCKADDR *)&temp_sock, &temp_sock_size);
	
	cout << inet_ntoa(temp_sock.sin_addr) << endl;

	while (true)
	{
		char buffer[1024];
		int len;
		if (recv(NewConnection, (char*)&len, sizeof(int), 0) != sizeof(int))
			break;
		int ret = recv(NewConnection, buffer, len, 0);
		buffer[len] = '\0';
		if (strcmp(buffer, "end") == 0)
		{
			strncpy_s(buffer, "Server is closed", sizeof("Server is closed"));
			len = strlen(buffer);
			send(NewConnection, (char *)&len, sizeof(int), 0);
			send(NewConnection, buffer, len, 0);
			break;
		}
		if (ret != len)
		{
			cout << "disconnected" << endl;
			break;
		}
		cout << buffer << ", len " << ret << endl;

	}

	closesocket(NewConnection); // ���� ������ �ݴ´�.
	closesocket(ListeningSocket); // ������ ������ �ݴ´�.

	// Ŭ���̾�Ʈ�� ������ ��ٸ�

	if (WSACleanup() == SOCKET_ERROR) {
		printf("WASCleanup failed with error %d\n", WSAGetLastError());
	}
}