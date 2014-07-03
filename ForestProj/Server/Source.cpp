#include <cstdio>
#include <WinSock2.h>

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

	closesocket(NewConnection); // ���� ������ �ݴ´�.
	closesocket(ListeningSocket); // ������ ������ �ݴ´�.

	// Ŭ���̾�Ʈ�� ������ ��ٸ�

	if (WSACleanup() == SOCKET_ERROR) {
		printf("WASCleanup failed with error %d\n", WSAGetLastError());
	}
}