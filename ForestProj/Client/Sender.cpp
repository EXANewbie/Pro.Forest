﻿#include <cstdio>
#include <conio.h>
#include <WinSock2.h>
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
#include "../protobuf/userattack.pb.h"
#include "../protobuf/userattackresult.pb.h"

void send_move(const SOCKET s, const char& c, const int& myID);
void copy_to_buffer(char *buf, int* type, int* len, std::string* content);
void receiver(const SOCKET s, int* myID);

void Sender(const SOCKET sock, int* myID, Character* myChar)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	int type;
	int len;
	char *buf = new char[1024000];
	fflush(stdin);
	while (true)
	{
		char c;
		c = _getch();

		Scoped_Wlock SW1(&chars->srw);
		Scoped_Wlock SW2(&mons->srw);

		if (myChar->getPrtHp() == 0)
		{
			printf("아직 캐릭터가 생성되지 않았습니다..\n");
			continue;
		}

		if (c == 'x' || c == 'X')
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

			send(sock, buf, len + sizeof(int)* 2, 0);
			break;
		}
		else if (c == 'w' || c == 'W' || c == 'a' || c == 'A' || c == 's' || c == 'S' || c == 'd' || c == 'D')
		{
			send_move(sock, c, *myID);
		}
		else if (c == 'i' || c == 'I')
		{
			//내정보 보여주자.
			printf("##내 정보:\n");
			if (myChar->getID() == -1)
			{
				printf("아직 캐릭터가 생성되지 않았습니다.\n잠시만 기다려 주십시오\n\n");
				continue;
			}
			{
				printf("이름(ID) : %s(%d) 위치 : %d, %d\n레벨 : %d 체력(현재/최고량) : %d/%d 공격력 : %d 경험치[현재/목표] : [%d/%d]\n\n",
					myChar->getName().c_str(), myChar->getID(), myChar->getX(), myChar->getY(), myChar->getLv(), myChar->getPrtHp(), myChar->getMaxHp(), myChar->getPower(), myChar->getPrtExp(), myChar->getMaxExp());
			}
		}
		else if (c == 'j' || c == 'J')
		{
			//같은 방에 있는 유저들의 정보를 보여주자.
			printf("##동료 정보:\n");

			int size = chars->size();
			if (size <= 1)
			{
				printf("현재 같은방에 동료가 없습니다\n\n");
				continue;
			}
			printf("--현재원 %d명\n", size - 1);
			for (auto iter = chars->begin(); iter != chars->end(); ++iter)
			{
				Character* other = iter->second;
				if (other->getID() == *myID) continue;
				printf("이름(ID) : %s(%d) 위치 : %d, %d\n레벨 : %d 체력(현재/최고량) : %d/%d 공격력 : %d 현재 경험치 : %d \n\n",
					other->getName().c_str(), other->getID(), other->getX(), other->getY(), other->getLv(), other->getPrtHp(), other->getMaxHp(), other->getPower(), other->getPrtExp());
			}
		}
		else if (c == 'o' || c == 'O')
		{
			//같은 방에 있는 몬스터들의 정보를 보여주자.

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
				printf("이름(ID) : %s(%d) 위치 : %d, %d\n레벨 : %d 체력(현재/최고량) : %d/%d 공격력 : %d\n\n",
					mon->getName().c_str(), mon->getID(), mon->getX(), mon->getY(), mon->getLv(), mon->getPrtHp(), mon->getMaxHp(), mon->getPower());
			}

		}
		else if (c == 'q' || c == 'Q')
		{
			// 공격을 하도록 하자!
			int size = mons->size();
			if (size == 0)
			{
				printf("현재 같은방에 공격할 몬스터가 없습니다\n\n");
				continue;
			}
			//가장 체력이 약한놈부터 때려잡는다!
			Monster* weakMon = NULL;
			int weakHp = 987654321;

			for (auto iter = mons->begin(); iter != mons->end(); ++iter)
			{
				if (weakHp > iter->second->getPrtHp())
				{
					weakMon = iter->second;
					weakHp = weakMon->getPrtHp();
				}
			}

			USER_ATTACK::CONTENTS userattackContents;
			auto userattack = userattackContents.add_data();
			userattack->set_attcktype(1);
			userattack->set_id_m(weakMon->getID());
			userattack->set_x_m(weakMon->getX());
			userattack->set_y_m(weakMon->getY());

			std::string bytestring;
			userattackContents.SerializeToString(&bytestring);
			type = PUSER_ATTCK;
			len = bytestring.length();
			copy_to_buffer(buf, &type, &len, &bytestring);

			if (send(sock, buf, len + sizeof(int)* 2, 0) == SOCKET_ERROR)
			{
				printf("시스템 오류!공격실패\n");
			}
			else
			{
				printf("몬스터 %s(%d)를 공격하였습니다!\n", weakMon->getName().c_str(), weakMon->getID());
			}

		}
	}
}