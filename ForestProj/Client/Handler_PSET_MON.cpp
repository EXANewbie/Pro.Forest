#include <WinSock2.h>

#include "Scoped_Lock.h"
#include "cmap.h"
#include "mmap.h"

#include "../protobuf/setmonster.pb.h"

void Handler_PSET_MON(int *myID, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	SET_MONSTER::CONTENTS setmonsterContents;
	setmonsterContents.ParseFromString(*str);

	Scoped_Wlock SW(&mons->srw);

	for (int i = 0; i < setmonsterContents.data_size(); ++i)
	{
		auto tmpMon = setmonsterContents.data(i);
		std::string monName = tmpMon.name();
		Monster* mon;
		if (monName == "기사")
		{
			mon = new Knight(tmpMon.id());
		}
		// 일단 else문 달음. 이것 안하면 c4703에러뜸
		else
		{
			mon = new Knight();
		}
		mon->setX(tmpMon.x());
		mon->setY(tmpMon.y());
		mon->setLv(tmpMon.lv(), tmpMon.maxhp(), tmpMon.power());
		
		mons->insert(tmpMon.id(), mon);
		
		printf("※ 앗!야생의 %s [ID:%d]가 나타났습니다!\n", mon->getName().c_str(), mon->getID());
	}
	setmonsterContents.clear_data();
}