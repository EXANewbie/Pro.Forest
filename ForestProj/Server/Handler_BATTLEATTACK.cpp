#include <string>

#include "../protobuf/battleattack.pb.h"
#include "../protobuf/monsterattackresult.pb.h"
#include "../protobuf/userrespawn.pb.h"

#include "Completion_Port.h"
#include "DMap.h"
#include "Scoped_Lock.h"
#include "Memory_Pool.h"
#include "DMap_monster.h"
#include "msg.h"
#include "TimerThread.h"

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

void make_monster_vector_in_room(Character* myChar, vector<Monster *>& send_list, bool autolocked);
void make_vector_id_in_room(E_List *, vector<Character *>&);
void send_message(msg, vector<Character *> &, bool);
void unpack(msg, char *, int *);

void Handler_BATTLEATTACK(LPPER_IO_DATA ioInfo, string* readContents) {
	BATTLEATTACK::CONTENTS battleattack;

	battleattack.ParseFromString(*readContents);

	if (ioInfo->block != nullptr)
	{
		Memory_Pool::getInstance()->pushBlock(ioInfo->block);
		ioInfo->block = nullptr;
	}
	ioInfo_Pool::getInstance()->pushBlock(ioInfo);

	auto FVEC_M = F_Vector_Mon::getInstance();
	int leaveUserSize, x, y;
	int attackType;
	vector<int> clist;

	string bytestring;
	int len = 0;

	int ID_m = battleattack.id();

	auto AMAP_MON = Access_Map_Mon::getInstance();
//	auto AMAP = Access_Map::getInstance();
	Monster* monster;

	monster = AMAP_MON->find(ID_m);
	//���� ���� ���¿� ��Ŷ�� ���°� ��ġ���� �ʽ��ϴ�!!
	{
		Scoped_Rlock MONSTER_READ_LOCK(monster->getLock());
		if (monster->getState() != PMODEBATTLEATTACK)
		{
			return;
		}
		x = monster->getX();
		y = monster->getY();
	}

	vector<int> damage;
	vector<Character *> nxt, receiver;
	auto elist = F_Vector::getInstance()->get(x, y);
	auto elist_m = F_Vector_Mon::getInstance()->get(x, y);
	{
		Scoped_Wlock E_LIST_WRITE_LOCK(&elist->slock);
		Scoped_Wlock E_LIST_MON_WRITE_LOCK(&elist_m->slock);
		// AI�� ���� ���� �������� ����մϴ�.
		monster->getAttackInfo(ATTACKSTART, vector<int>(), &attackType, nxt, damage);

		// ���� �濡 �����ϰ� �ִ� �������� ������ �����ɴϴ�.
		make_vector_id_in_room(elist, receiver);

		MONSTER_ATTACK_RESULT::CONTENTS monsterattackresultContents;
		monsterattackresultContents.set_id_m(monster->getID());
		monsterattackresultContents.set_attacktype(attackType);

		for (int i = 0; i < nxt.size(); i++)
		{
			{
				Scoped_Wlock CHARACTER_WRITE_LOCK(nxt[i]->getLock());
				nxt[i]->attacked(damage[i]);

				if (nxt[i]->getPrtHp() == 0)
				{
					elist->erase(nxt[i]);

					int Respawn_Time = nxt[i]->getLv() * 2000; // ������ �ð��� ���⼭ ����
					USER_RESPAWN::CONTENTS userrespawnContents;
					userrespawnContents.set_id(nxt[i]->getID());
					userrespawnContents.set_x(22); // �������� x�� ��ġ�� ����
					userrespawnContents.set_y(22); // �������� y�� ��ġ�� ����
					userrespawnContents.SerializeToString(&bytestring);

					auto MemoryPool = Memory_Pool::getInstance();
					auto blocks = MemoryPool->popBlock();

					unpack(msg(USERRESPAWN, bytestring.size(), bytestring.c_str()), blocks->getBuffer(), &len);

					Timer::getInstance()->addSchedule(Respawn_Time, string(blocks->getBuffer(), len));

					MemoryPool->pushBlock(blocks);
				}

				auto data = monsterattackresultContents.add_data();
				data->set_id(nxt[i]->getID());
				data->set_prthp(nxt[i]->getPrtHp());
			}

			if (monsterattackresultContents.data_size() == ATTACKED_USER_MAXIMUM)
			{
				monsterattackresultContents.SerializeToString(&bytestring);
				send_message(msg(PMONSTER_ATTACK_RESULT, bytestring.size(), bytestring.c_str()), receiver, false);

				monsterattackresultContents.clear_data();
				bytestring.clear();
			}

			// ü���� 0�� �Ǵ� �������� �����ϴ� �κ��Դϴ�.
		}
		monsterattackresultContents.SerializeToString(&bytestring);
		send_message(msg(PMONSTER_ATTACK_RESULT, bytestring.size(), bytestring.c_str()), receiver, false);

		monsterattackresultContents.clear_data();
		leaveUserSize = elist->size();

		if (leaveUserSize == 0)
		{
			auto elist_m = FVEC_M->get(x, y);
			for (auto itr = elist_m->begin(); itr != elist_m->end(); itr++)
			{
				(*itr)->SET_PEACE_MODE();
			}
		}
		else
		{
			monster->CONTINUE_BATTLE_MODE(clist, attackType);
		}
	}
}