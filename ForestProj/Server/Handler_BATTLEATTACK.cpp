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

	int ID_m = battleattack.id();

	auto AMAP_MON = Access_Map_Mon::getInstance();
	auto AMAP = Access_Map::getInstance();
	Monster* monster;
	{
		monster = AMAP_MON->find(ID_m);
		//지금 현재 상태와 패킷의 상태가 일치하지 않습니다!!
		if (monster->getState() != PMODEBATTLEATTACK)
		{
			return;
		}

		x = monster->getX();
		y = monster->getY();

		vector<int> damage;
		vector<Character *> nxt, receiver;
		auto elist = F_Vector::getInstance()->get(x, y);
		{
			// AI를 통해 대상과 데미지를 계산합니다.
			monster->getAttackInfo(ATTACKSTART, vector<int>(), &attackType, nxt, damage);

			// 현재 방에 참여하고 있는 유저들의 정보를 가져옵니다.
			make_vector_id_in_room(elist, receiver);

			MONSTER_ATTACK_RESULT::CONTENTS monsterattackresultContents;
			monsterattackresultContents.set_id_m(monster->getID());
			monsterattackresultContents.set_attacktype(attackType);


			string bytestring;


			for (int i = 0; i < nxt.size(); i++)
			{
				if (nxt[i] == nullptr) {
					printf("???\n");
					exit(0);
				}
				nxt[i]->attacked(damage[i]);

				auto data = monsterattackresultContents.add_data();
				data->set_id(nxt[i]->getID());
				data->set_prthp(nxt[i]->getPrtHp());

				if (monsterattackresultContents.data_size() == ATTACKED_USER_MAXIMUM)
				{
					monsterattackresultContents.SerializeToString(&bytestring);
					send_message(msg(PMONSTER_ATTACK_RESULT, bytestring.size(), bytestring.c_str()), receiver, false);

					monsterattackresultContents.clear_data();
					bytestring.clear();
				}

				// 체력이 0이 되는 유저들을 제거하는 부분입니다.
				if (nxt[i]->getPrtHp() == 0)
				{
					elist->erase(nxt[i]);

					int Respawn_Time = nxt[i]->getLv() * 2000; // 리스폰 시간은 여기서 정의
					string bytestring;
					USER_RESPAWN::CONTENTS userrespawnContents;
					userrespawnContents.set_id(nxt[i]->getID());
					userrespawnContents.set_x(22); // 리스폰될 x의 위치를 결정
					userrespawnContents.set_y(22); // 리스폰될 y의 위치를 결정
					userrespawnContents.SerializeToString(&bytestring);

					auto MemoryPool = Memory_Pool::getInstance();
					auto blocks = MemoryPool->popBlock();

					int len = 0;
					unpack(msg(USERRESPAWN, bytestring.size(), bytestring.c_str()), blocks->getBuffer(), &len);

					Timer::getInstance()->addSchedule(Respawn_Time, string(blocks->getBuffer(), len));

					MemoryPool->pushBlock(blocks);
				}
			}
			monsterattackresultContents.SerializeToString(&bytestring);
			send_message(msg(PMONSTER_ATTACK_RESULT, bytestring.size(), bytestring.c_str()), receiver, false);

			monsterattackresultContents.clear_data();
			leaveUserSize = elist->size();
			for (int i = 0; i < ( nxt.size() > 50 ? 50 : nxt.size() ); i++)
			{
				clist.push_back(nxt[i]->getID());
			}
		}
	}

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