#include <string>

#include "../protobuf/deadrespawn.pb.h"
#include "../protobuf/setmonster.pb.h"

#include "Completion_Port.h"
#include "DMap.h"
#include "Scoped_Lock.h"
#include "Memory_Pool.h"
#include "DMap_monster.h"
#include "msg.h"

/*
#include "Check_Map.h"
#include "types.h"
#include "Completion_Port.h"
#include "character.h"
#include "Sock_set.h"
#include "DMap.h"
#include "Scoped_Lock.h"

#include "Memory_Pool.h"
#include "monster.h"
#include "DMap_monster.h"
#include "msg.h"
*/
using std::string;

void make_vector_id_in_room(E_List *, vector<Character *>&);
void send_message(msg, vector<Character *> &, bool);
void unpack(msg, char *, int *);

void Handler_DEADRESPAWN(LPPER_IO_DATA ioInfo, string* readContents) {
	DEADRESPAWN::CONTENTS deadmsg;
	deadmsg.ParseFromString(*readContents);

	if (ioInfo->block != nullptr)
	{
		Memory_Pool::getInstance()->pushBlock(ioInfo->block);
		ioInfo->block = nullptr;
	}
	ioInfo_Pool::getInstance()->pushBlock(ioInfo);

	auto FVEC_M = F_Vector_Mon::getInstance();

	int ID = deadmsg.id();

	auto AMAP_MON = Access_Map_Mon::getInstance();
	Monster* monster;

	monster = AMAP_MON->find(ID);

	printLog("ID : %d\n", ID);
	//���� ���� ���¿� ��Ŷ�� ���°� ��ġ���� �ʽ��ϴ�!!
	if (monster->getState() != DEAD)
	{
		return;
	}

	E_List* elist;
	E_List_Mon* elist_mon;

	{
		Scoped_Wlock MONSTER_WRITE_LOCK(monster->getLock());
		monster->setHPMax();
		int x = monster->getX();
		int y = monster->getY();
		elist_mon = FVEC_M->get(x, y);
		elist = F_Vector::getInstance()->get(x, y);
	}
	
	{
		Scoped_Wlock E_LIST_WRITE_LOCK(&elist->slock);
		Scoped_Wlock E_LIST_MON_WRITE_LOCK(&elist_mon->slock);

		// step 1. ���͸� �ش� �濡 ��ġ�մϴ�.
		elist_mon->push_back(monster);

		// step 2. �ش� ���� �����鿡�� ���Ͱ� ��Ÿ������ �˸��ϴ�.
		vector<Character *> receiver;
		make_vector_id_in_room(elist, receiver);

		string bytestring;
		SET_MONSTER::CONTENTS setmonsterContents;
		auto data = setmonsterContents.add_data();
		{
			Scoped_Rlock MONSTER_READ_LOCK(monster->getLock());
			data->set_id(monster->getID());
			data->set_name(monster->getName());
			data->set_lv(monster->getLv());
			data->set_maxhp(monster->getMaxHp());
			data->set_power(monster->getPower());
			data->set_x(monster->getX());
			data->set_y(monster->getY());
		}

		setmonsterContents.SerializeToString(&bytestring);
		auto message = msg(PSET_MON, bytestring.size(), bytestring.c_str());

		send_message(message, receiver, false);
		
		// step 3. ���� �濡 ������ �����ϸ� ��Ʋ ���·�, ������ ��ȭ ���·�!!
		if (elist->size() > 0)
		{
			monster->SET_BATTLE_MODE();
		}
		else
		{
			monster->SET_PEACE_MODE();
		}
	}
}