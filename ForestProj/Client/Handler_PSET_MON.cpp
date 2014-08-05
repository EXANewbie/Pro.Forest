#include <WinSock2.h>

#include "Scoped_Lock.h"
#include "cmap.h"
#include "mmap.h"

#include "../protobuf/setmonster.pb.h"

void Handler_PSET_MON(Character *myChar, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	SET_MONSTER::CONTENTS setmonsterContents;
	setmonsterContents.ParseFromString(*str);

	for (int i = 0; i < setmonsterContents.data_size(); ++i)
	{
		auto tmpMon = setmonsterContents.data(i);
		int monName = tmpMon.name();
		Monster* mon;
		if (monName == 1)
		{
			mon = new Knight(tmpMon.id());
		}
		// �ϴ� else�� ����. �̰� ���ϸ� c4703������
		else
		{
			mon = new Knight();
		}
		mon->setX(tmpMon.x());
		mon->setY(tmpMon.y());
		mon->setLv(tmpMon.lv(), tmpMon.maxhp(), tmpMon.power());
		{
			Scoped_Wlock SW(&mons->srw);
			mons->insert(tmpMon.id(), mon);
		}

		printf("�߻��� %s�� [id : %d (%d, %d) ]��Ÿ�����ϴ�!\n", mon->getName().c_str(), mon->getID(), mon->getX(), mon->getY());
	}
	setmonsterContents.clear_data();
}