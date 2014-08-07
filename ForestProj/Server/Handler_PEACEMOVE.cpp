#include <string>

#include "../protobuf/peacemove.pb.h"
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

void Handler_PEACEMOVE(LPPER_IO_DATA ioInfo, string* readContents)
{
	PEACEMOVE::CONTENTS peacemove;
	peacemove.ParseFromString(*readContents);

	if (ioInfo->block != nullptr)
	{
		Memory_Pool::getInstance()->pushBlock(ioInfo->block);
		ioInfo->block = nullptr;
	}
	ioInfo_Pool::getInstance()->pushBlock(ioInfo);

	auto FVEC = F_Vector::getInstance();
	auto FVEC_M = F_Vector_Mon::getInstance();

	int ID_m = peacemove.id();

	auto AMAP_MON = Access_Map_Mon::getInstance();
	Monster* monster;

	if (ID_m == 0)
	{
		printf("왜?\n");
		exit(0);
	}

	monster = AMAP_MON->find(ID_m);

	if (monster == nullptr)
	{
		//몬스터가 유효하지 않습니다.

		printf("$$$$$ID : %d, (%d)\n", ID_m, peacemove.state());
		exit(0);
		return;
	}

	printLog("ID : %d\n", ID_m);

	int bef_x_off, bef_y_off;
	int nxt_x_off, nxt_y_off;

	E_List *now_elist, *next_elist;
	E_List_Mon *now_elist_m, *next_elist_m;
	bool ordered = false;
	//지금 현재 상태와 패킷의 상태가 일치하지 않습니다!!
	{
		Scoped_Rlock MONSTER_READ_LOCK(monster->getLock());
		if (monster->getState() != PMODEPEACEMOVE)
		{
			return;
		}
		bef_x_off = peacemove.xoff();
		bef_y_off = peacemove.yoff();

		monster->getNextOffset(bef_x_off, bef_y_off, &nxt_x_off, &nxt_y_off);

		int x = monster->getX();
		int y = monster->getY();

		now_elist = FVEC->get(x, y);
		now_elist_m = FVEC_M->get(x, y);

		next_elist = FVEC->get(x + nxt_x_off, y + nxt_y_off);
		next_elist_m = FVEC_M->get(x + nxt_x_off, y + nxt_y_off);

		if (nxt_x_off + nxt_y_off < 0)
		{
			ordered = false;
		}
		else
		{
			ordered = true;
		}
	}

	{
		E_List *first, *second;
		E_List_Mon *first_m, *second_m;

		if (ordered == true)
		{
			first = now_elist;
			first_m = now_elist_m;

			second = next_elist;
			second_m = next_elist_m;
		}
		else
		{
			first = next_elist;
			first_m = next_elist_m;

			second = now_elist;
			second_m = now_elist_m;
		}

		Scoped_Wlock NOW_E_LIST_WRITE_LOCK(&first->slock);
		Scoped_Wlock NOW_E_LIST_MON_WRITE_LOCK(&first_m->slock);

		Scoped_Wlock NEXT_E_LIST_WRITE_LOCK(&second->slock);
		Scoped_Wlock NEXT_E_LIST_MON_WRITE_LOCK(&second_m->slock);

		if (monster->getState() != PMODEPEACEMOVE)
		{
			return;
		}

		now_elist_m->erase(monster);

		{
			Scoped_Wlock MONSTER_WRITE_LOCK(monster->getLock());
			monster->setX(monster->getX() + nxt_x_off);
			monster->setY(monster->getY() + nxt_y_off);
		}

		next_elist_m->push_back(monster);

		int size = next_elist->size();

		// 유저가 존재합니다!! W.A.R.N.I.N.G !! W.A.R.N.I.N.G !!
		if (size > 0)
		{
			vector<Character *> receiver;
			make_vector_id_in_room(next_elist, receiver);

			string bytestring;
			SET_MONSTER::CONTENTS setmonsterContents;
			auto data = setmonsterContents.add_data();
			{
				Scoped_Wlock MONSTER_WRITE_LOCK(monster->getLock());
				data->set_id(monster->getID());
				data->set_name(monster->getName());
				data->set_lv(monster->getLv());
				data->set_maxhp(monster->getMaxHp());
				data->set_power(monster->getPower());
				data->set_x(monster->getX());
				data->set_y(monster->getY());

				monster->SET_BATTLE_MODE();
				// 여기서부터 배틀모드가 되었다는걸 타이머에게 전송해줘요~
			}

			setmonsterContents.SerializeToString(&bytestring);
			auto message = msg(PSET_MON, bytestring.size(), bytestring.c_str());

			send_message(message, receiver, false);
			// 여기에 모든 유저들에게 나의 존재를 알려줘야되요~
		}
		else
		{
			{
				Scoped_Wlock MONSTER_WRITE_LOCK(monster->getLock());
				monster->CONTINUE_PEACE_MODE(nxt_x_off, nxt_y_off);
				// 이동해도 되요~~ ^-^
			}
		}
	}
}