#include <WinSock2.h>
#include <memory>

#include "cmap.h"
#include "mmap.h"
#include "types.h"

void Handler_PSET_USER(int *myID, std::string* str);
void Handler_PINIT(int* myID, Character *myChar, std::string* str);
void Handler_PMOVE_USER(int *myID, std::string* str);
void Handler_PERASE_USER(int *myID, std::string* str);
void Handler_PSET_MON(int *myID, std::string* str);
void Handler_PERASE_MON(int *myID, std::string* str);
void Handler_PUSER_ATTCK_RESULT(int *myID, std::string* str);
void Handler_PUSER_SET_LV(int *myID, std::string* str);
void Handler_PUSER_SET_EXP(int *myID, std::string* str);
void Handler_PMONSTER_ATTACK_RESULT(int *myID, std::string* str);

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
		int inc = 0;
		do
		{
			int end = recv(s, ptr.get()+inc, len-inc, 0);
			inc += end;
		} while (inc < len);
		std::string tmp(ptr.get(), len);

		if (type == PSET_USER)
		{
			Handler_PSET_USER(myID, &tmp);
		}
		else if (type == PINIT)
		{
			Handler_PINIT(myID, myChar, &tmp);
		}
		else if (type == PMOVE_USER)
		{
			Handler_PMOVE_USER(myID, &tmp);
		}
		else if (type == PERASE_USER)
		{
			Handler_PERASE_USER(myID, &tmp);
		}
		else if (type == PSET_MON)
		{
			Handler_PSET_MON(myID, &tmp);
		}
		else if (type == PERASE_MON)
		{
			Handler_PERASE_MON(myID, &tmp);
		}
		else if (type == PUSER_ATTCK_RESULT)
		{
			Handler_PUSER_ATTCK_RESULT(myID, &tmp);
		}
		else if (type == PUSER_SET_LV)
		{
			Handler_PUSER_SET_LV(myID, &tmp);
		}
		else if (type == PUSER_SET_EXP)
		{
			Handler_PUSER_SET_EXP(myID, &tmp);
		}
		else if (type == PMONSTER_ATTACK_RESULT)
		{
			//내일할 곳.
			Handler_PMONSTER_ATTACK_RESULT(myID, &tmp);
		}

		tmp.clear();
	}

}