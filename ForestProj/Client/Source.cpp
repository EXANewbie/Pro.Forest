#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include <conio.h>

#include "character.h"
#include "cmap.h"

#define PORT 78911
#define SERVER_IP_ADDRESS /*"localhost"*/"10.1.7.206"/*206"*/
enum packetType{ CONNECT, INIT, SET_USER, MOVE_USER, DISCONN, ERASE_USER };

void send_move(const SOCKET s,const char& c,const int& myID);


void receiver(const SOCKET s, int* myID, SYNCHED_CHARACTER_MAP* chars)
{
	char buf[1024];
	packetType type;
	int len;
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

		if (type == SET_USER)
		{
			char *pBuf = buf;
			while (len > 0)
			{
				int id, x, y;
				for (int i = 0; i < 3; ++i)
				{
					int* param[] = { &id, &x, &y };
					memcpy(param[i], pBuf, sizeof(int));
					pBuf += sizeof(int);
				}

				Character other;
				other.setID(id);
				other.setX(x);
				other.setY(y);
				chars->insert(id, other);

				printf("your char id : %d  (%d,%d)\n", id, x, y);
				len -= (3 * sizeof(int));
			}
		}
		else if (type == INIT)
		{
			int id, x, y;
			char *pBuf = buf;
			for (int i = 0; i < 3; ++i)
			{
				int* param[] = { &id, &x, &y };
				memcpy(param[i], pBuf, sizeof(int));
				pBuf += sizeof(int);
			}
			*myID = id;
			Character myCharacter;
			myCharacter.setID(id);
			myCharacter.setX(x);
			myCharacter.setY(y);
			chars->insert(id,myCharacter);

			printf("my char id : %d, (%d,%d)\n", id, myCharacter.getX(), myCharacter.getY());
		}
		else if (type == MOVE_USER)
		{
			int id, x_off, y_off;
			char *pBuf = buf;
			while (len > 0)
			{
				for (int i = 0; i < 3; ++i)
				{
					int* param[] = { &id, &x_off, &y_off };
					memcpy(param[i], pBuf, sizeof(int));
					pBuf += sizeof(int);
				}
				len -= (sizeof(int)*3);

				Character* other;
				other = (chars->find(id));
				other->setX(other->getX() + x_off);
				other->setY(other->getY() + y_off);

				printf("your char id! : %d, (%d,%d) \n", other->getID(), other->getX(), other->getY());
			}
		}
		else if (type == ERASE_USER)
		{
			char *pBuf = buf;
			while (len > 0)
			{
				int id;
				memcpy(&id, pBuf, sizeof(int));
				pBuf += sizeof(int);
				len -= sizeof(int);

				chars->erase(id);
				printf("your char id erase! : %d \n", id);
			}
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
	//strncpy_s(buf, "HELLO SERVER!",13);
	printf("%d\n", sizeof("HELLO SERVER!"));
	memcpy(buf, "HELLO SERVER!", sizeof("HELLO SERVER!"));
	len = strlen(buf);
	send(s, (char*)&type, sizeof(int), 0);
	send(s, (char*)&len, sizeof(int), 0);
	send(s, buf, len, 0);
	
	//자신의 캐릭터 생성
	int myID;
	SYNCHED_CHARACTER_MAP *chars = new SYNCHED_CHARACTER_MAP();

	// 데이터 수신 쓰레드 동작.
	std::thread t(receiver, s, &myID, chars);
	
	while (true)
	{
		char c;
		c=_getch();
		if (c == 'x')
		{
			type = DISCONN;
			memcpy(buf, "BYE SERVER!", sizeof("BYE SERVER!"));
			len = strlen(buf);
			send(s, (char*)&type, sizeof(int), 0);
			send(s, (char*)&len, sizeof(int), 0);
			send(s, buf, len, 0);
			break;
		}
		else if (c == 'w'||c=='a'||c=='s'||c=='d')
		{
			send_move(s, c, myID);
		}
	}

	t.join();
	closesocket(s);
}

void send_move(const SOCKET s, const char& c, const int& myID)
{
	int x_off, y_off;
	char buf[1024];
	int len;
	packetType type = MOVE_USER;

	if (c == 'w') {	x_off = 0;y_off = -1;}
	else if (c == 'a'){	x_off = -1;	y_off = 0;}
	else if (c == 's'){	x_off = 0;	y_off = 1;}
	else if (c == 'd'){	x_off = 1;	y_off = 0;}
	
	char* pBuf = buf;
	memcpy(pBuf, &myID, sizeof(int));
	pBuf += sizeof(int);
	memcpy(pBuf, &x_off, sizeof(int));
	pBuf += sizeof(int);
	memcpy(pBuf, &y_off, sizeof(int));
	pBuf += sizeof(int);
	len = sizeof(int)* 3;

	send(s, (char*)&type, sizeof(int), 0);
	send(s, (char*)&len, sizeof(int), 0);
	send(s, buf, len, 0);

}