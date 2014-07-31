#include <memory>
#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include <conio.h>
#include <string>

#include "character.h"
#include "cmap.h"
#include "types.h"
#include "Scoped_Lock.h"

#include "../protobuf/connect.pb.h""
#include "../protobuf/disconn.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"

struct deleter {
	void operator()(char *c){ delete[]c; }
};

void receiver(const SOCKET s, int* myID)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
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
		recv(s, (char*)&len, sizeof(int), 0);

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
				int lv = user.lv(), maxHp = user.maxhp(), power = user.power(), exp = user.exp();
				other->setID(id);
				other->setX(x);
				other->setY(y);
				other->setLv(lv,maxHp,power);
				other->setExp(exp);
				{
					Scoped_Wlock(&chars->srw);
					chars->insert(id, other);
				}
				printf("He char id : %d  (%d,%d)\n", other->getID(), other->getX(), other->getY());
				printf("His lv : %d, hp : %d, power : %d, exp : %d\n", other->getLv(), other->getPrtHp(), other->getPower(), other->getExp());
			}
			contents.clear_data();
		}
		else if (type == PINIT)
		{
			INIT::CONTENTS contents;
			contents.ParseFromString(tmp);

			auto user = contents.data(0);
			int id = user.id(), x = user.x(), y = user.y();
			int lv = user.lv(), maxHp = user.maxhp(), power = user.power(), exp = user.exp();
			*myID = id;
			Character* myCharacter = new Character;
			myCharacter->setID(id);
			myCharacter->setX(x);
			myCharacter->setY(y);
			myCharacter->setLv(lv,maxHp,power);
			myCharacter->setExp(exp);
			{
				Scoped_Wlock(&chars->srw);
				chars->insert(id, myCharacter);
			}
			printf("내가 돌아왔다\n");
			printf("My char id : %d, (%d,%d)\n", id, myCharacter->getX(), myCharacter->getY());
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
				Character* myChar;
				{
					Scoped_Rlock(&chars->srw);
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
					Scoped_Wlock(&chars->srw);
					chars->erase(id);
				}
				printf("His char id erase! : %d \n", id);
			}
			contents.clear_data();
		}

	}

}