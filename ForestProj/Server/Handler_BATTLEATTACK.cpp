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

void make_vector_id_in_room(E_List *, vector<Character *>&);
void send_message(msg, vector<Character *> &, bool);
void unpack(msg, char *, int *);

void Handler_BATTLEATTACK(LPPER_IO_DATA ioInfo, string* readContents) {
	auto FVEC_M = F_Vector_Mon::getInstance();

	if (ioInfo->block != nullptr)
	{
		Memory_Pool::getInstance()->pushBlock(ioInfo->block);
	}
	ioInfo_Pool::getInstance()->pushBlock(ioInfo);
	BATTLEATTACK::CONTENTS battleattack;

	battleattack.ParseFromString(*readContents);
	int ID = battleattack.id();

	auto AMAP_MON = Access_Map_Mon::getInstance();
	auto AMAP = Access_Map::getInstance();
	Monster* monster;
	{
		Scoped_Rlock SR(&AMAP->slock);
		Scoped_Rlock SR2(&AMAP_MON->slock);
		monster = AMAP_MON->find(ID);
		//지금 현재 상태와 패킷의 상태가 일치하지 않습니다!!
		if (monster->getState() != battleattack.state())
		{
			return;
		}

		vector<int> damage;
		vector<Character *> nxt, receiver;
		auto elist = F_Vector::getInstance()->get(monster->getX(), monster->getY());
		int attackType;
		{
			Scoped_Wlock SW(&elist->slock);

			// AI를 통해 대상과 데미지를 계산합니다.
			monster->getAttackInfo(Monster::ATTACKSTART, vector<int>(), &attackType, nxt, damage);

			// 현재 방에 참여하고 있는 유저들의 정보를 가져옵니다.
			make_vector_id_in_room(elist, receiver);

			MONSTER_ATTACK_RESULT::CONTENTS monsterattackresultContents;
			monsterattackresultContents.set_id(monster->getID());
			monsterattackresultContents.set_attacktype(attackType);


			string bytestring;
			monsterattackresultContents.SerializeToString(&bytestring);
			
			monsterattackresultContents.clear_data();

			// 체력이 0이 되는 유저들을 제거하는 부분입니다.
			for (int i = 0; i < nxt.size(); i++)
			{
				Scoped_Wlock SW(nxt[i]->getLock());
				nxt[i]->attacked(damage[i]);

				auto data = monsterattackresultContents.add_data();
				data->set_id(nxt[i]->getID());
				data->set_damage(nxt[i]->getPrtHp());

				if (monsterattackresultContents.data_size() < ATTACKED_USER_MAXIMUM)
				{
					monsterattackresultContents.SerializeToString(&bytestring);
					send_message(msg(PMONSTER_ATTACK_RESULT, bytestring.size(), bytestring.c_str()), receiver, false);

					monsterattackresultContents.clear_data();
					bytestring.clear();
				}

				if (nxt[i]->getPrtHp() == 0)
				{
					elist->erase(nxt[i]);

					int Respawn_Time = receiver[i]->getLv() * 2000; // 리스폰 시간은 여기서 정의
					string bytestring;
					USER_RESPAWN::CONTENTS userrespawnContents;
					userrespawnContents.set_id(receiver[i]->getID());
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
		}

		if (elist->size() == 0)
		{
			monster->SET_PEACE_MODE();
		}
		else
		{
			vector<int> clist;
			for (int i = 0; i < nxt.size(); i++)
			{
				clist.push_back(nxt[i]->getID());
			}

			monster->CONTINUE_BATTLE_MODE(clist,attackType);
		}
	}
}