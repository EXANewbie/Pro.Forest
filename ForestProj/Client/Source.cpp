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
		int y=recv(s, buf, len, 0);
		buf[y] = '\0';
		printf("%s\n", buf);
	}
}

void main(void)
{

	WSADATA wsaData;
	SOCKET s;
	SOCKADDR_IN ServerAddr;
	// ?덉냽 2.2濡?珥덇린??

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// ?대씪?댁뼵?몄슜 ?뚯폆 ?앹꽦

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);

	// ?쒕쾭???곌껐

	connect(s, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr));
	printf("connection success");

	// ?곗씠???섏떊 ?곕젅???숈옉.
	std::thread t(receiver, s);

	// ?곗씠???≪떊 遺遺?
	
	char buf[1024]="up down left right";
	
	while (true)
	{
		int len = strlen(buf);
		send(s, (char*)&len, sizeof(int), 0); // message header transfer
		send(s, buf, len, 0);//message body transgfer
	}
	
	
	t.join();
	closesocket(s);
}