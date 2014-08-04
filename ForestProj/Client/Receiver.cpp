#include <memory>
#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include <conio.h>
#include <string>
#include <iostream>

#include "character.h"
#include "cmap.h"
#include "types.h"
#include "Scoped_Lock.h"
#include "monster.h"
#include "mmap.h"

#include "../protobuf/connect.pb.h""
#include "../protobuf/disconn.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/setmonster.pb.h"
#include "../protobuf/erasemonster.pb.h"

#include "../protobuf/userattackresult.pb.h"
#include "../protobuf/setuserlv.pb.h"
#include "../protobuf/setuserexp.pb.h"

struct deleter {
	void operator()(char *c){ delete[]c; }
};

void receiver(const SOCKET s, int* myID, Character* myChar)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	char *buf;
	TYPE type;
	int len;
	while (true)
	{
		int chk = recv(s, (char*)&type, sizeof(int), 0);
		if (chk != sizeof(int)) {
			printf("disconnected\n");
			break;
		}
		int chk2=recv(s, (char*)&len, sizeof(int), 0);

		std::shared_ptr <char> ptr(new char[len], deleter());
		int end = recv(s, ptr.get(), len, 0);
		std::string tmp(ptr.get(), len);

		if (type == PSET_USER)
		{
			SET_USER::CONTENTS contents;
			contents.ParseFromString(tmp);
			for (int i = 0; i<contents.data_size(); ++i)
			{
				auto user = contents.data(i);
				Character* other = new Character;
				int id = user.id(), x = user.x(), y = user.y();
				int lv = user.lv(), maxHp = user.maxhp(), power = user.power(), prtExp = user.prtexp(), maxExp=user.maxexp();
				other->setID(id);
				other->setX(x);
				other->setY(y);
				other->setLv(lv,maxHp,power,maxExp);
				other->setPrtExp(prtExp);
				{
					Scoped_Wlock SW(&chars->srw);
					chars->insert(id, other);
				}
				printf("동료[ %d ]가 왔습니다. ", other->getID());
				printf("레벨 : %d, 체력 : %d, 공격력 : %d, 경혐치 : %d\n", other->getLv(), other->getPrtHp(), other->getPower(), other->getPrtExp());
			}
			contents.clear_data();
		}
		else if (type == PINIT)
		{
			INIT::CONTENTS contents;
			contents.ParseFromString(tmp);

			auto user = contents.data(0);
			int id = user.id(), x = user.x(), y = user.y();
			int lv = user.lv(), maxHp = user.maxhp(), power = user.power(), maxexp = user.maxexp();
			*myID = id;
			Character* myCharacter = new Character();
			myCharacter->setID(id);
			myCharacter->setX(x);
			myCharacter->setY(y);
			myCharacter->setLv(lv,maxHp,power,maxexp);
			*myChar = *myCharacter;
			{
				Scoped_Wlock SW(&chars->srw);
				chars->insert(id, myChar);
			}
			printf("My char id : %d, (%d,%d)\n", id, myChar->getX(), myChar->getY());
			//printf("my lv : %d, hp : %d, power : %d, exp : %d\n",myCharacter->getLv(),myCharacter->getPrtHp(),myCharacter->getPower(),myCharacter->getExp());
			contents.clear_data();

		}
		else if (type == PMOVE_USER)
		{
			MOVE_USER::CONTENTS contents;
			contents.ParseFromString(tmp);

			for (int i = 0; i<contents.data_size(); ++i)
			{
				auto user = contents.data(i);
				int id = user.id(), x_off = user.xoff(), y_off = user.yoff();
				
				if (x_off == 0 && y_off == 0) {
					printf("몬스터와 전투중입니다. 움직일수 없습니다.\n");
					break;
				}

				Character* myChar;
				{
					Scoped_Rlock SR(&chars->srw);
					myChar = (chars->find(id));
				}
				myChar->setX(myChar->getX() + x_off);
				myChar->setY(myChar->getY() + y_off);

				printf("I am moved! My char id : %d, (%d,%d) \n", myChar->getID(), myChar->getX(), myChar->getY());
				//printf("my lv : %d, hp : %d, power : %d, exp : %d\n", myChar->getLv(), myChar->getPrtHp(), myChar->getPower(), myChar->getExp());
			}
			contents.clear_data();
		}
		else if (type == PERASE_USER)
		{
			ERASE_USER::CONTENTS contents;
			contents.ParseFromString(tmp);

			for (int i = 0; i<contents.data_size(); ++i)
			{
				auto user = contents.data(i);
				int id = user.id();
				{
					Scoped_Wlock SW(&chars->srw);
					chars->erase(id);
				}
				printf("His char id erase! : %d \n", id);
			}
			contents.clear_data();
		}
		else if (type == PSET_MON)
		{
			SET_MONSTER::CONTENTS setmonsterContents;
			setmonsterContents.ParseFromString(tmp);

			for (int i = 0; i < setmonsterContents.data_size(); ++i)
			{
				auto tmpMon = setmonsterContents.data(i);
				int monName = tmpMon.name();
				Monster* mon;
				if (monName == 1)
				{
					mon = new Knight(tmpMon.id());
				}
				// 일단 else문 달음. 이것 안하면 c4703에러뜸
				else
				{
					mon = new Knight();
				}
				mon->setX(tmpMon.x());
				mon->setY(tmpMon.y());
				mon->setLv(tmpMon.lv(), tmpMon.maxhp(), tmpMon.power());
				{
					Scoped_Wlock SW(&mons->srw);
					mons->insert(tmpMon.id(), mon);
				}
				
				printf("야생의 %s가 [id : %d (%d, %d) ]나타났습니다!\n",mon->getName().c_str(), mon->getID(), mon->getX(), mon->getY());
			}
			setmonsterContents.clear_data();
		}
		
		else if (type == PERASE_MON)
		{
			ERASE_MONSTER::CONTENTS erasemonsterContents;
			erasemonsterContents.ParseFromString(tmp);

			for (int i = 0; i<erasemonsterContents.data_size(); ++i)
			{
				auto mon = erasemonsterContents.data(i);
				int id = mon.id();
				std::string monName;
				{
					Scoped_Rlock SR(&mons->srw);
					monName = mons->find(id)->getName();
				}
				{
					Scoped_Wlock SW(&mons->srw);
					mons->erase(id);
				}
				printf("몬스터[%s]를 지나쳤다! : %d \n", monName.c_str(),id);
			}
			erasemonsterContents.clear_data();
		}

		else if (type == PUSER_ATTCK_RESULT)
		{
			USER_ATTACK_RESULT::CONTENTS userattackresultContents;
			userattackresultContents.ParseFromString(tmp);

			//for문을 한 것은 다중 공격을 하였을 경우를 위해.
			for (int i = 0; i < userattackresultContents.data_size(); ++i)
			{
				auto userattackresult = userattackresultContents.data(i);
				int id = userattackresult.id();
				int attckType = userattackresult.attcktype();
				int id_m = userattackresult.id_m();
				int prtHp_m = userattackresult.prthp_m();

				Monster* targetMon;
				{
					Scoped_Rlock SR(&mons->srw);
					targetMon = mons->find(id_m);
				}
				int damage;
				{
					Scoped_Wlock SW(targetMon->getLock());
					damage = targetMon->getPrtHp() - prtHp_m;
					targetMon->setPrtHp(prtHp_m);
				}

				if (targetMon->getPrtHp() == 0)
				{
					printf("**유저 [%d]가 기술 %d을 이용하여 몬스터%s[%d]를 %d만큼 공격하여 죽였습니다.\n",
						id, attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
					{
						Scoped_Wlock SW(&mons->srw);

						//몬스터에 대해서는 lock을 걸 필요가 없는듯 한데..
						mons->erase(targetMon->getID());
					}
				}
				else
				{
					//이름도 붙여주고 싶다. protobuf string 넘겨줘도 문제없게 하고싶음.
					printf("유저 [%d]가 기술 %d을 이용하여 몬스터 %s[%d]를 %d만큼 공격하였습니다.\n",
						id, attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
				}
			}
			
		}
		else if (type == PUSER_SET_LV)
		{
			SET_USER_LV::CONTENTS setuserlvContents;
			setuserlvContents.ParseFromString(tmp);

			{
				Scoped_Rlock SR(&chars->srw);
				for (int i = 0; i < setuserlvContents.data_size(); ++i)
				{
					auto setuserlv = setuserlvContents.data(i);
					int id = setuserlv.id();
					int lv = setuserlv.lv();
					int maxHp = setuserlv.maxhp();
					int power = setuserlv.power();
					int expUp = setuserlv.expup();
					int maxexp = setuserlv.maxexp();

					Character* lvUpChar = chars->find(id);
					if (lvUpChar == NULL) printf("나 나오면 안돼는데 나옴?");
					else
					{
						Scoped_Wlock SW(lvUpChar->getLock());
						lvUpChar->setExpUp(expUp);
						lvUpChar->setLv(lv, maxHp, power, maxexp);
						printf("유저 [%d]가 레벨이 %d로 올랐습니다!!\n", id, lv);
					}
				}
			}
		}
		else if (type == PUSER_SET_EXP)
		{
			SET_USER_EXP::CONTENTS setuserexpContents;
			setuserexpContents.ParseFromString(tmp);
			auto setuserexp = setuserexpContents.data(0);
			
			int id = setuserexp.id();
			int expUp = setuserexp.expup();

			Character* expUpChar = chars->find(id);
			{
				Scoped_Wlock Sw(expUpChar->getLock());
				expUpChar->setExpUp(expUp);
			}
			printf("유저 [%d]가 경험치가 상승했습니다!!\n", expUpChar->getID());
		}
		else if (type == PMONSTER_ATTACK_RESULT)
		{
			//내일할 곳.
		}

		tmp.clear();
	}

}