#include <string>

#include "../protobuf/peacemove.pb.h"

#include "Completion_Port.h"
#include "DMap.h"
#include "Scoped_Lock.h"
#include "Memory_Pool.h"
#include "DMap_monster.h"

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

void Handler_PEACEMOVE(LPPER_IO_DATA ioInfo, string* readContents) {
	auto FVEC_M = F_Vector_Mon::getInstance();

	if (ioInfo->block != nullptr)
	{
		Memory_Pool::getInstance()->pushBlock(ioInfo->block);
	}
	ioInfo_Pool::getInstance()->pushBlock(ioInfo);
	PEACEMOVE::CONTENTS peacemove;

	peacemove.ParseFromString(*readContents);
	int ID = peacemove.id();

	auto AMAP_MON = Access_Map_Mon::getInstance();
	Monster* monster;
	{
		Scoped_Rlock SR(&AMAP_MON->slock);
		monster = AMAP_MON->find(ID);
		//지금 현재 상태와 패킷의 상태가 일치하지 않습니다!!
		if (monster->getState() != peacemove.state())
		{
			return;
		}

	}

	{
		Scoped_Wlock SW(monster->getLock());
		int bef_x_off, bef_y_off;

		if (peacemove.has_xoff() == true && peacemove.has_yoff() == true)
		{
			bef_x_off = peacemove.xoff();
			bef_y_off = peacemove.yoff();
		}
		else
		{
			bef_x_off = 0;
			bef_y_off = 0;
		}

		int nxt_x_off = 0, nxt_y_off = 0;

		monster->getNextOffset(bef_x_off, bef_y_off, &nxt_x_off, &nxt_y_off);

		// 더 이상 움직일 곳이 없는 경우 ㅠ.ㅠ
		if (nxt_x_off == 0 && nxt_y_off == 0)
		{

		}
		else
		{
			{
				auto elist = FVEC_M->get(monster->getX(), monster->getY());
				Scoped_Wlock(&elist->slock);
				elist->erase(monster);
			}
			monster->setX(monster->getX() + nxt_x_off);
			monster->setY(monster->getY() + nxt_y_off);
			{
				auto elist = FVEC_M->get(monster->getX(), monster->getY());
				Scoped_Wlock(&elist->slock);
				elist->push_back(monster);
			}



			auto elist = F_Vector::getInstance()->get(monster->getX(), monster->getY());
			Scoped_Rlock(&elist->slock);
			int size = elist->size();

			// 유저가 존재합니다!! W.A.R.N.I.N.G !! W.A.R.N.I.N.G !!
			if (size > 0)
			{

				monster->SET_BATTLE_MODE();
				// 여기서부터 배틀모드가 되었다는걸 타이머에게 전송해줘요~
			}
			else
			{
				monster->CONTINUE_PEACE_MODE(nxt_x_off, nxt_y_off);
				// 이동해도 되요~~ ^-^
			}
		}
	}

}