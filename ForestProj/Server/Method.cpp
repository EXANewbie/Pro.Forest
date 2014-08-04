#include <string>

#include "../protobuf/peacemove.pb.h"
#include "../protobuf/battleattack.pb.h"

#include "Memory_Pool.h"
#include "TimerThread.h"
#include "Scoped_Lock.h"
#include "monster.h"
#include "DMap.h"
#include "msg.h"

using std::string;

bool Boundary_Check(const int, const int, const int, int, int);
void make_vector_id_in_room(E_List *, vector<Character*>&);
void unpack(msg, char *, int *);

void Knight::getNextOffset(int bef_x_off, int bef_y_off, int *nxt_x_off, int *nxt_y_off)
{
	int Points[][2] = { { 1, 0 }, { 0, -1 }, { -1, 0 }, { 0, 1 } };
	int size = 4;
//	for (int i = 0; i < 4;i++) 
	*nxt_x_off = bef_x_off;
	*nxt_y_off = bef_y_off;

	if (bef_x_off == 0 && bef_y_off == 0) {
		
	}
	else if (Boundary_Check(ID, x, y, *nxt_x_off, *nxt_y_off) == true)
	{
		return; // 
	}

	int *cur[] = { nxt_x_off, nxt_y_off };
	int nowPos = 0;
	for (int i = 0; i < size; i++)
	{
		nowPos = i;
		if (*cur[0] == Points[i][0] && *cur[1] == Points[i][1])
			break;
	}

	for (int i = 0; i < size; i++)
	{
		nowPos = (nowPos + 1) % size;
		*cur[0] = Points[nowPos][0];
		*cur[1] = Points[nowPos][1];
		if (Boundary_Check(ID, x, y, *nxt_x_off, *nxt_y_off) == true)
		{
			return;
		}
	}

	*cur[0] = 0, *cur[1] = 0;

	return;
}

void Knight::getAttackInfo(int attacktype, const vector<int>& befusers, int *nextattacktype, vector<int>& nextusers, vector<int>& damage)
{
	// 멀티샷을 후려쳤거나, 한명을 두방 후려쳤다면
	if (attacktype == MULTIATTACK || attacktype == DOUBLEATTACK)
	{
		getBASICATTACKINFO(nextattacktype, nextusers, damage);
	}
	// 그 전에 일반 공격을 했다면
	else
	{
		int rand = bigRand() % 4;
		// 50프로 확률로 다시 일반 공격
		if (rand < 2)
		{
			getBASICATTACKINFO(nextattacktype, nextusers, damage);
		}
		// 25프로 확률로 데미지 2배 공격
		else if (rand == 2)
		{
			getDOUBLEATTACKINFO(nextattacktype, nextusers, damage);
		}
		// 25프로 확률로 데미지 1/3으로 전체 공격
		else
		{
			getMULTIATTACKINFO(nextattacktype, nextusers, damage);
		}
	}
}

void Knight::getRespawnTime(int* time) {
	*time = 20000;
}

void Knight::SET_BATTLE_MODE() {
	if (state == BATTLE)
		return;

	state = BATTLE;
	BATTLEATTACK::CONTENTS battlemsg;
	battlemsg.set_id(ID);
	battlemsg.set_state(BATTLE);

	int time = 1000;
	string message;
	battlemsg.SerializePartialToString(&message);
	auto MemoryPool = Memory_Pool::getInstance();
	auto blocks = MemoryPool->popBlock();
	int len = 0;
	unpack(msg(PMODEBATTLEATTACK, message.size(), message.c_str()), blocks->getBuffer(), &len);

	Timer::getInstance()->addSchedule(time, string(blocks->getBuffer(), len));

	MemoryPool->pushBlock(blocks);
}

void Knight::SET_PEACE_MODE() {
	if (state == PEACE)
		return;

	state = PEACE;
	PEACEMOVE::CONTENTS peacemsg;
	peacemsg.set_id(ID);
	peacemsg.set_state(PEACE);

	int time = 1000;
	string message;
	peacemsg.SerializePartialToString(&message);
	auto MemoryPool = Memory_Pool::getInstance();
	auto blocks = MemoryPool->popBlock();
	int len = 0;
	unpack(msg(PMODEPEACEMOVE, message.size(), message.c_str()), blocks->getBuffer(), &len);

	Timer::getInstance()->addSchedule(time, string(blocks->getBuffer(), len));

	MemoryPool->pushBlock(blocks);
}

void Knight::CONTINUE_BATTLE_MODE(vector<int> users, int attack_type) {
	BATTLEATTACK::CONTENTS battlemsg;
	battlemsg.set_id(ID);
	battlemsg.set_state(BATTLE);
	battlemsg.set_attacktype(attack_type);
	battlemsg.set_finished(0);

	int time = 1000;

	for (int i = 0; i < users.size(); i++) {
		auto target = battlemsg.add_data();
		target->set_id(users[i]);

		if (battlemsg.data_size() == TARGET_USER_MAXIMUM) {
			string message;
			battlemsg.SerializeToString(&message);
			auto MemoryPool = Memory_Pool::getInstance();
			auto blocks = MemoryPool->popBlock();
			int len = 0;
			unpack(msg(PMODEBATTLEATTACK, message.size(), message.c_str()), blocks->getBuffer(), &len);

			Timer::getInstance()->addSchedule(time, string(blocks->getBuffer(), len));

			MemoryPool->pushBlock(blocks);
			battlemsg.clear_data();
		}
	}

	battlemsg.set_finished(1);

	string message;
	battlemsg.SerializeToString(&message);
	auto MemoryPool = Memory_Pool::getInstance();
	auto blocks = MemoryPool->popBlock();
	int len = 0;
	unpack(msg(PMODEBATTLEATTACK, message.size(), message.c_str()), blocks->getBuffer(), &len);

	Timer::getInstance()->addSchedule(time, string(blocks->getBuffer(), len));
	
	MemoryPool->pushBlock(blocks);
}

void Knight::CONTINUE_PEACE_MODE(int nxt_x_off, int nxt_y_off) {
	PEACEMOVE::CONTENTS peacemsg;
	peacemsg.set_id(ID);
	peacemsg.set_state(PEACE);
	peacemsg.set_xoff(nxt_x_off);
	peacemsg.set_yoff(nxt_y_off);

	int time = 1000;
	string message;
	peacemsg.SerializeToString(&message);
	auto MemoryPool = Memory_Pool::getInstance();
	auto blocks = MemoryPool->popBlock();
	int len = 0;
	unpack(msg(PMODEPEACEMOVE, message.size(), message.c_str()), blocks->getBuffer(), &len);

	Timer::getInstance()->addSchedule(time, string(blocks->getBuffer(), len));

	MemoryPool->pushBlock(blocks);
}

void Knight::getBASICATTACKINFO(int *nextattacktype, vector<int>& nextusers, vector<int>& damage)
{
	*nextattacktype = BASICATTACK; // 평타 설정
	int nextTarget = UNDEFINED, minHP;

	auto FVEC = F_Vector::getInstance();
	auto elist = FVEC->get(x, y);
	{
		Scoped_Rlock(&elist->slock);
		for (auto itr = elist->begin(); itr != elist->end(); itr++)
		{
			auto chars = *itr;
			Scoped_Rlock(chars->getLock());

			if (nextTarget == UNDEFINED || minHP > chars->getPrtHp())
			{
				nextTarget = chars->getID();
				minHP = chars->getPrtHp();
			}
		}
		int realdamage = power; // 데미지 계산은 여기에서!
		damage.push_back(power);
		nextusers.push_back(nextTarget);
	}
}

void Knight::getDOUBLEATTACKINFO(int *nextattacktype, vector<int>& nextusers, vector<int>& damage)
{
	*nextattacktype = DOUBLEATTACK; // 평타 설정
	int nextTarget = UNDEFINED, minHP;

	auto FVEC = F_Vector::getInstance();
	auto elist = FVEC->get(x, y);
	{
		Scoped_Rlock(&elist->slock);
		for (auto itr = elist->begin(); itr != elist->end(); itr++)
		{
			auto chars = *itr;
			Scoped_Rlock(chars->getLock());

			if (nextTarget == UNDEFINED || minHP > chars->getPrtHp())
			{
				nextTarget = chars->getID();
				minHP = chars->getPrtHp();
			}
		}
		int realdamage = 2*power; // 데미지 계산은 여기에서!
		damage.push_back(power);
		nextusers.push_back(nextTarget);
	}
}

void Knight::getMULTIATTACKINFO(int *nextattacktype, vector<int>& nextusers, vector<int>& damage)
{
	*nextattacktype = MULTIATTACK; // 평타 설정

	auto FVEC = F_Vector::getInstance();
	auto elist = FVEC->get(x, y);
	{
		Scoped_Rlock(&elist->slock);
		for (auto itr = elist->begin(); itr != elist->end(); itr++)
		{
			auto chars = *itr;
			int realdamage = power / 3; // 데미지 공식은 여기서 변경하세요!
			Scoped_Rlock(chars->getLock());

			nextusers.push_back(chars->getID());
			damage.push_back(realdamage);
		}
	}
}