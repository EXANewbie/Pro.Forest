#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include <conio.h>
#include <string>

#include "character.h"
#include "cmap.h"

#include "../protobuf/connect.pb.h""
#include "../protobuf/disconn.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"


#define PORT 78911

#define SERVER_IP_ADDRESS /*"localhost"*/"10.1.7.206"

enum packetType{ PCONNECT, PINIT, PSET_USER, PMOVE_USER, PDISCONN, PERASE_USER };

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

		if (type == PSET_USER)
		{
			SET_USER::CONTENTS contents;
			contents.ParseFromString(buf);

			for (int i = 0; i<contents.data_size(); ++i)
			{
				auto user = contents.data(i);
				Character other;
				int id = user.id(), x = user.x(), y = user.y();
				other.setID(id);
				other.setX(x);
				other.setY(y);
				chars->insert(id, other);

				printf("your char id : %d  (%d,%d)\n", id, x, y);
			}
		}
		else if (type == PINIT)
		{
			INIT::CONTENTS contents;
			contents.ParseFromString(buf);

			auto user = contents.data(0);
			int id = user.id(), x = user.x(), y = user.y();
			
			*myID = id;
			Character myCharacter;
			myCharacter.setID(id);
			myCharacter.setX(x);
			myCharacter.setY(y);
			chars->insert(id, myCharacter);
			
			printf("my char id : %d, (%d,%d)\n", id, myCharacter.getX(), myCharacter.getY());
		}
		else if (type == PMOVE_USER)
		{
			MOVE_USER::CONTENTS contents;
			contents.ParseFromString(buf);

			for (int i = 0; i<contents.data_size(); ++i)
			{
				auto user = contents.data(i);
				int id = user.id(), x_off = user.xoff(), y_off = user.yoff();
				Character* other;
				other = (chars->find(id));
				other->setX(other->getX() + x_off);
				other->setY(other->getY() + y_off);

				printf("your char id! : %d, (%d,%d) \n", other->getID(), other->getX(), other->getY());
			}

		}
		else if (type == PERASE_USER)
		{
			ERASE_USER::CONTENTS contents;
			contents.ParseFromString(buf);

			for (int i = 0; i<contents.data_size(); ++i)
			{
				auto user = contents.data(i);
				int id = user.id();
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
	printf("connection success\n");


	// 데이터 송신 부분
	int type;
	int len;
	char buf[1024] ; 

	//PCONNECT 전송.
	type = PCONNECT;
	
	char* pBuf = buf;
	
	CONNECT::CONTENTS contents;
	std::string* buff_msg = contents.mutable_data();
	*buff_msg = "HELLO SERVER!";
	std::string bytestring;
	contents.SerializeToString(&bytestring);
	
	len = bytestring.length();
	
	memcpy(pBuf, (char *)&type, sizeof(int));
	pBuf += sizeof(int);
	memcpy(pBuf, (char *)&len, sizeof(int));
	pBuf += sizeof(int);
	memcpy(pBuf, &bytestring, len);

	send(s, buf, len + sizeof(int) * 2, 0);
	
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
			char* pBuf = buf;
			//PDISCONN 전송
			type = PDISCONN;
			
			DISCONN::CONTENTS contents;
			std::string* buff_msg = contents.mutable_data();
			*buff_msg = "BYE SERVER!";
			std::string bytestring;
			contents.SerializeToString(&bytestring);

			len = bytestring.length();
			
			memcpy(pBuf, (char*)&type, sizeof(int));
			pBuf += sizeof(int);
			memcpy(pBuf, (char *)&len, sizeof(int));
			pBuf += sizeof(int);
			memcpy(pBuf, bytestring.c_str(), len);

			send(s, buf, len+sizeof(int), 0);
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
	packetType type = PMOVE_USER;

	if (c == 'w') {	x_off = 0;y_off = -1;}
	else if (c == 'a'){	x_off = -1;	y_off = 0;}
	else if (c == 's'){	x_off = 0;	y_off = 1;}
	else if (c == 'd'){	x_off = 1;	y_off = 0;}
	
	MOVE_USER::CONTENTS contents;
	auto element = contents.mutable_data()->Add();
	element->set_id(myID);
	element->set_xoff(x_off);
	element->set_yoff(y_off);
		
	std::string bytestring;
	contents.SerializeToString(&bytestring);
	len = bytestring.length();

	char* pBuf = buf;
	memcpy(pBuf, &type, sizeof(int));
	pBuf += sizeof(int);
	memcpy(pBuf, &len, sizeof(int));
	pBuf += sizeof(int);
	memcpy(pBuf, bytestring.c_str(), len);
	
	send(s, buf, len+sizeof(int)*2, 0);
}