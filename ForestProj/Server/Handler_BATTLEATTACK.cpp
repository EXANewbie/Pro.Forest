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
		//���� ���� ���¿� ��Ŷ�� ���°� ��ġ���� �ʽ��ϴ�!!
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

			// AI�� ���� ���� �������� ����մϴ�.
			monster->getAttackInfo(Monster::ATTACKSTART, vector<int>(), &attackType, nxt, damage);

			// ���� �濡 �����ϰ� �ִ� �������� ������ �����ɴϴ�.
			make_vector_id_in_room(elist, receiver);

			MONSTER_ATTACK_RESULT::CONTENTS monsterattackresultContents;
			monsterattackresultContents.set_id_m(monster->getID());
			monsterattackresultContents.set_attacktype(attackType);


			string bytestring;
			monsterattackresultContents.SerializeToString(&bytestring);
			
			monsterattackresultContents.clear_data();

			// ü���� 0�� �Ǵ� �������� �����ϴ� �κ��Դϴ�.
			for (int i = 0; i < nxt.size(); i++)
			{
				Scoped_Wlock SW(nxt[i]->getLock());
				nxt[i]->attacked(damage[i]);

				auto data = monsterattackresultContents.add_data();
				data->set_id(nxt[i]->getID());
				data->set_prthp(nxt[i]->getPrtHp());

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

					int Respawn_Time = receiver[i]->getLv() * 2000; // ������ �ð��� ���⼭ ����
					string bytestring;
					USER_RESPAWN::CONTENTS userrespawnContents;
					userrespawnContents.set_id(receiver[i]->getID());
					userrespawnContents.set_x(22); // �������� x�� ��ġ�� ����
					userrespawnContents.set_y(22); // �������� y�� ��ġ�� ����
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