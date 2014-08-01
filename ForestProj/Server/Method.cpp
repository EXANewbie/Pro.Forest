#include <string>

#include "../protobuf/peacemove.pb.h"

#include "Memory_Pool.h"
#include "TimerThread.h"
#include "monster.h"
#include "DMap.h"
#include "msg.h"

using std::string;

bool Boundary_Check(const int, const int, const int, int, int);
void make_vector_id_in_room(E_List *, vector<int>&);
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

void Knight::SET_BATTLE_MODE() {
	state = BATTLE;
}

void Knight::SET_PEACE_MODE() {
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

void Knight::CONTINUE_BATTLE_MODE(int user_id, int attack_type) {

}

void Knight::CONTINUE_PEACE_MODE(int nxt_x_off, int nxt_y_off) {
	PEACEMOVE::CONTENTS peacemsg;
	peacemsg.set_id(ID);
	peacemsg.set_state(PEACE);
	peacemsg.set_xoff(nxt_x_off);
	peacemsg.set_yoff(nxt_y_off);

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