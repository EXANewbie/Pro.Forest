#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include <conio.h>
#include <string>

#include "character.h"
#include "cmap.h"
#include "types.h"
#include "Scoped_Lock.h"
#include "mmap.h"

#include "../protobuf/connect.pb.h""
#include "../protobuf/disconn.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/setmonster.pb.h"
#include "../protobuf/erasemonster.pb.h"

#define PORT 78911

#define SERVER_IP_ADDRESS /*"localhost"*/"10.1.7.10"

SYNCHED_CHARACTER_MAP* SYNCHED_CHARACTER_MAP::instance;
SYNCHED_MONSTER_MAP* SYNCHED_MONSTER_MAP::instance;

void send_move(const SOCKET s,const char& c,const int& myID);
void copy_to_buffer(char *buf, int* type, int* len, std::string* content);
void receiver(const SOCKET s, int* myID);

void main(void)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

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
	char *buf = new char[1024000] ; 

	//PCONNECT 전송.
	type = PCONNECT;
	
	char* pBuf = buf;
	
	CONNECT::CONTENTS contents;
	std::string* buff_msg = contents.mutable_data();
	*buff_msg = "HELLO SERVER!";
/*	for (int i = 0; i < 5000; i++)		//패킷 사이즈를 약 50kb로 쐇을 때 끊겨오는 현상 발견, 테스트용으로 넣음
		(*buff_msg).append(" Hi Hello!");*/ // 만약 이 테스트를 하고 싶다면 BLOCK_SIZE = 1 << 17;로 수정 권고
	std::string bytestring;
	contents.SerializeToString(&bytestring);
	len = bytestring.length();
	
	copy_to_buffer(buf, &type, &len, &bytestring);

	send(s, buf, len + sizeof(int) * 2, 0);
	
	//자신의 캐릭터 생성
	int myID;

	// 데이터 수신 쓰레드 동작.
	std::thread t(receiver, s, &myID);
	
	while (true)
	{
		char c;
		c=_getch();
		if (c == 'x')
		{
			//PDISCONN 전송
			type = PDISCONN;
			
			DISCONN::CONTENTS contents;
			std::string* buff_msg = contents.mutable_data();
			*buff_msg = "BYE SERVER!";
			std::string bytestring;
			contents.SerializeToString(&bytestring);

			len = bytestring.length();
			
			copy_to_buffer(buf, &type, &len, &bytestring);

			send(s, buf, len+sizeof(int)*2, 0);
			break;
		}
		else if (c == 'w'||c=='a'||c=='s'||c=='d')
		{
			send_move(s, c, myID);
		}
		else if (c == 'q')// attack!!
		{

		}
		else if (c == 'i')
		{
			//내정보 보여주자.
			printf("##내 정보:\n");
			{
				Scoped_Rlock SR(&chars->srw);
				Character* me = chars->find(myID);
				if (me == NULL)
				{
					printf("아직 캐릭터가 생성되지 않았습니다.\n잠시만 기다려 주십시오\n\n");
					continue;
				}
				
				printf("ID : %d 위치 : %d, %d\n레벨 : %d 체력(현재/최고량) : %d/%d 공격력 : %d\n\n", me->getID(), me->getX(), me->getY(), me->getLv(), me->getPrtHp(), me->getMaxHp(), me->getPower());
			}
		}
		else if (c == 'j')
		{
			//같은 방에 있는 유저들의 정보를 보여주자.
			printf("##동료 정보:\n");
			{
				Scoped_Rlock SR(&chars->srw);
				int size = chars->size();
				if (size == 1) 
				{
					printf("현재 같은방에 동료가 없습니다\n\n"); 
					continue;
				}
				printf("--현재원 %d명\n", size-1);
				for (auto iter = chars->begin(); iter != chars->end(); ++iter)
				{
					Character* other = iter->second;
					if (other->getID() == myID) continue;
					printf("ID : %d 위치 : %d, %d\n레벨 : %d 체력(현재/최고량) : %d/%d 공격력 : %d\n\n", 
						other->getID(), other->getX(), other->getY(), other->getLv(), other->getPrtHp(), other->getMaxHp(), other->getPower());
				}
			}
		}
		else if (c == 'o')
		{
			//같은 방에 있는 몬스터들의 정보를 보여주자.
			{
				Scoped_Rlock SR(&mons->srw);
				int size = mons->size();
				if (size == 0)
				{
					printf("현재 같은방에 몬스터가 없습니다\n\n");
					continue;
				}
				printf("--몬스터 총 %d마리\n", size);
				for (auto iter = mons->begin(); iter != mons->end(); ++iter)
				{
					Monster* mon = iter->second;
					printf("이름(ID) : %s (%d) 위치 : %d, %d\n레벨 : %d 체력(현재/최고량) : %d/%d 공격력 : %d\n\n",
						mon->getName().c_str(), mon->getID(), mon->getX(), mon->getY(), mon->getLv(), mon->getPrtHp(), mon->getMaxHp(), mon->getPower());
				}
			}
		}
		else if (c == 'k')
		{
		}
	}

	t.join();
	closesocket(s);
}

