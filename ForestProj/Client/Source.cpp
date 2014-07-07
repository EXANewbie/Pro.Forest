#include <cstdio>
#include <WinSock2.h>
#include <thread>

#include "character.h"

#define PORT 78911
#define SERVER_IP_ADDRESS /*"localhost"*/"10.1.7.206"
enum packetType{ CONNECT, INIT, SET_USER, MOVE_USER, DISCONN, ERASE_USER };


void receiver(SOCKET& s,Character& myCharacter)
{
	char buf[1024];
	int len;
	packetType type;
	while (true)
	{
		int chk=recv(s, (char*)&type, sizeof(int), 0);
		if (chk != sizeof(int)) {
			printf("disconnected\n");
			break;
		}
		recv(s, (char*)&len, sizeof(int), 0);		

		int end = recv(s, buf, len, 0);
		buf[end] = '\0';

		int indx = 0;
		int id;
		for (int i = 0; i < 4; ++i)
		{
			((char*)&id)[i] = buf[indx++];
		}
		
		if (type == SET_USER)
		{
			int x, y;
			for (int i = 0; i < 4; ++i)
				((char*)&x)[i] = buf[indx++];
			
			for (int i = 0; i < 4; ++i)
				((char*)&y)[i] = buf[indx++];
			
			printf("diff id : %d  (%d,%d)",id,x,y);
		}
		else if (type == INIT)
		{
			int x, y;
			for (int i = 0; i < 4; ++i)
				((char*)&x)[i] = buf[indx++];
			
			for (int i = 0; i < 4; ++i)
				((char*)&y)[i] = buf[indx++];

			myCharacter.setID(id);
			myCharacter.setX(x);
			myCharacter.setY(y);
			printf("my id : %d, (%d,%d)", id, myCharacter.getX(), myCharacter.getY());
		}
		
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


	// 데이터 송신 부분
	int type;
	int len;
	char buf[1024] ; 

	//CONNECT 전송.
	type = CONNECT;
	strncpy_s(buf, "HELLO SERVER!",13);
	len = strlen(buf);
	send(s, (char*)&type, sizeof(int), 0);
	send(s, (char*)&len, sizeof(int), 0);
	send(s, buf, len, 0);


	//자신의 캐릭터 생성
	Character myCharacter;
	
	// 데이터 수신 쓰레드 동작.
	std::thread t(receiver, s, myCharacter);
	


	t.join();
	closesocket(s);
}