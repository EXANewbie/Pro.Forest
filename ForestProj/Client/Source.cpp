#include <cstdio>
#include <WinSock2.h>
#include <thread>

#define PORT 78911
#define SERVER_IP_ADDRESS "10.1.7.206"

void receiver(SOCKET& s)
{
	char buf[1024];
	int len;
	while (true)
	{
		recv(s, (char*)&len, sizeof(int), 0);

		int y = recv(s, buf, len, 0);
		buf[y] = '\0';
		printf("%s\n", buf);
	}
}

void main(void)
{

	WSADATA wsaData;
	SOCKET s;
	SOCKADDR_IN ServerAddr;

	// 윈속 2.2로 초기화

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// 클라이언트용 소켓 생성

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);

	// 서버에 연결

	connect(s, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr));
	printf("connection success");

	// 데이터 수신 쓰레드 동작.
	std::thread t(receiver, s);

	// 데이터 송신 부분

	char buf[1024] = "up down left right";

	while (true)
	{
		int len = strlen(buf);
		send(s, (char*)&len, sizeof(int), 0); // message header transfer
		send(s, buf, len, 0);//message body transgfer
	}

	t.join();
	closesocket(s);
}