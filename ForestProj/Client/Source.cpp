#include <cstdio>
#include <WinSock2.h>

void main(void)
{
	const char *SERVER_IP_ADDRESS = "10.1.7.206";
	WSADATA wsaData;
	SOCKET s;
	SOCKADDR_IN ServerAddr;
	int Port = 78911;

	// ���� 2.2�� �ʱ�ȭ

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	// Ŭ���̾�Ʈ�� ���� ����

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	printf("%d\n", WSAGetLastError());

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(Port);
	ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);

	// ������ ����

	connect(s, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr));

	// ������ �۽� �κ�

	printf("%d\n", WSAGetLastError());

	closesocket(s);

}