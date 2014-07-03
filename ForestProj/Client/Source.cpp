#include <cstdio>
#include <WinSock2.h>

#define PORT 78911
#define SERVER_IP_ADDRESS "10.1.7.206"

void main(void)
{

	WSADATA wsaData;
	SOCKET s;
	SOCKADDR_IN ServerAddr;


	// 윈속 2.2로 초기화

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	// 클라이언트용 소켓 생성

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	printf("%d\n", WSAGetLastError());

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);

	// 서버에 연결

	connect(s, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr));


	printf("%d\n", WSAGetLastError());
	// 데이터 송신 부분
	printf("connection success");
	char buf[1024] = "hihi LJH";
	
	int t = 10;
	while (t--)
	{
		int len = strlen(buf);
		send(s, (char*)&len, sizeof(int), 0); // message header transfer
		int a=send(s, buf, len, 0);//message body transgfer
	}
	//end 전송.
	int ptr;
	strncpy_s(buf, "end", sizeof("end"));
	ptr = strlen(buf);
	send(s, (char *)&ptr, sizeof(int), 0);
	send(s, buf, ptr, 0);

	printf("메시지 기다림");
	recv(s, (char *)&ptr, sizeof(int), 0);
	recv(s, buf, ptr, 0);
	buf[ptr] = 0;

	printf("%s\n", buf);
	printf("%d\n", WSAGetLastError());

	closesocket(s);

}