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
	Monster* monster;
	{
		Scoped_Rlock SR(&AMAP_MON->slock);
		monster = AMAP_MON->find(ID);
		//지금 현재 상태와 패킷의 상태가 일치하지 않습니다!!
		if (monster->getState() != battleattack.state())
		{
			return;
		}

		vector<int> nxt, damage;
		int attackType;
		monster->getAttackInfo(Monster::ATTACKSTART, vector<int>(), &attackType, nxt, damage);

		auto elist = F_Vector::getInstance()->get(monster->getX(), monster->getY());
		Scoped_Wlock SW(&elist->slock);

		vector<Character *> receiver;
		make_vector_id_in_room(elist, receiver);

		MONSTER_ATTACK_RESULT::CONTENTS monsterattackresultContents;
		monsterattackresultContents.set_id(monster->getID());
		monsterattackresultContents.set_attacktype(attackType);
		for (int i = 0; i < receiver.size(); i++)
		{
			auto data = monsterattackresultContents.add_data();
			{
				Scoped_Wlock SW2(receiver[i]->getLock());
				receiver[i]->attacked(damage[i]);
				data->set_id(receiver[i]->getID());
				data->set_damage(receiver[i]->getPrtHp());
			}
			// 유저의 체력이 0인 경우 현재 방에서 제거하고, 리스폰 시간을 결정해서 타이머에 넣는다.
			{
				Scoped_Rlock SW2(receiver[i]->getLock());
				if (receiver[i]->getPrtHp() == 0) {
					elist->erase(receiver[i]);

					int Respawn_Time = receiver[i]->getLv() * 2000; // 리스폰 시간은 여기서 정의
					string bytestring;
					USER_RESPAWN::CONTENTS userrespawnContents;
					userrespawnContents.set_id(receiver[i]->getID());
					userrespawnContents.set_x(22); // 리스폰될 x의 위치를 결정
					userrespawnContents.set_y(22); // 리스폰될 y의 위치를 결정
					userrespawnContents.SerializeToString(&bytestring);
					
					auto MemoryPool = Memory_Pool::getInstance();
					auto blocks = MemoryPool->popBlock();

					blocks->getBuffer();
					int len = 0;
					unpack(msg(USERRESPAWN, bytestring.size(), bytestring.c_str()), blocks->getBuffer(), &len);

					Timer::getInstance()->addSchedule(Respawn_Time, string(blocks->getBuffer(), len));
				}
			}

			if (monsterattackresultContents.data_size() < ATTACKED_USER_MAXIMUM)
			{
				string bytestring;
				monsterattackresultContents.SerializeToString(&bytestring);
				send_message(msg(PMONSTER_ATTACK_RESULT, bytestring.size(), bytestring.c_str()), receiver, false);

				monsterattackresultContents.clear_data();
			}
		}

		string bytestring;
		monsterattackresultContents.SerializeToString(&bytestring);
		send_message(msg(PMONSTER_ATTACK_RESULT, bytestring.size(), bytestring.c_str()), receiver, false);

		monsterattackresultContents.clear_data();

		if (elist->size() == 0)
		{
			monster->SET_PEACE_MODE();
		}
		else
		{
			vector<int> clist;
			for (int i = 0; i < receiver.size(); i++)
			{
				clist.push_back(receiver[i]->getID());
			}

			monster->CONTINUE_BATTLE_MODE(clist,attackType);
		}
	}
}