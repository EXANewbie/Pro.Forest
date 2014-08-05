#include <WinSock2.h>
#include <memory>

#include "cmap.h"
#include "mmap.h"
#include "types.h"

void Handler_PSET_USER(Character *myChar, std::string* str);
void Handler_PINIT(int* myID, Character *myChar, std::string* str);
void Handler_PMOVE_USER(Character *myChar, std::string* str);
void Handler_PERASE_USER(Character *myChar, std::string* str);
void Handler_PSET_MON(Character *myChar, std::string* str);
void Handler_PERASE_MON(Character *myChar, std::string* str);
void Handler_PUSER_ATTCK_RESULT(Character *myChar, std::string* str);
void Handler_PUSER_SET_LV(Character *myChar, std::string* str);
void Handler_PUSER_SET_EXP(Character *myChar, std::string* str);
void Handler_PMONSTER_ATTACK_RESULT(Character *myChar, std::string* str);

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
			Handler_PSET_USER(myChar, &tmp);
		}
		else if (type == PINIT)
		{
			Handler_PINIT(myID, myChar, &tmp);
		}
		else if (type == PMOVE_USER)
		{
			Handler_PMOVE_USER(myChar, &tmp);
		}
		else if (type == PERASE_USER)
		{
			Handler_PERASE_USER(myChar, &tmp);
		}
		else if (type == PSET_MON)
		{
			Handler_PSET_MON(myChar, &tmp);
		}
		else if (type == PERASE_MON)
		{
			Handler_PERASE_MON(myChar, &tmp);
		}
		else if (type == PUSER_ATTCK_RESULT)
		{
			Handler_PUSER_ATTCK_RESULT(myChar, &tmp);
		}
		else if (type == PUSER_SET_LV)
		{
			Handler_PUSER_SET_LV(myChar, &tmp);
		}
		else if (type == PUSER_SET_EXP)
		{
			Handler_PUSER_SET_EXP(myChar, &tmp);
		}
		else if (type == PMONSTER_ATTACK_RESULT)
		{
			//내일할 곳.
			Handler_PMONSTER_ATTACK_RESULT(myChar, &tmp);
		}

		tmp.clear();
	}

}