#include <cstdio>
#include <WinSock2.h>

void main(void)
{
	const char *SERVER_IP_ADDRESS = "10.1.7.206";
	WSADATA wsaData;
	SOCKET s;
	SOCKADDR_IN ServerAddr;
	int Port = 78911;

	// 윈속 2.2로 초기화

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	// 클라이언트용 소켓 생성

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	printf("%d\n", WSAGetLastError());

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(Port);
	ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);

	// 서버에 연결

	connect(s, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr));

	// 데이터 송신 부분

	printf("%d\n", WSAGetLastError());

	closesocket(s);

}