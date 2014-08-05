#include <WinSock2.h>

#include "Scoped_Lock.h"
#include "cmap.h"
#include "mmap.h"

#include "../protobuf/moveuser.pb.h"

void Handler_PMOVE_USER(Character *myChar, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	MOVE_USER::CONTENTS contents;
	contents.ParseFromString(*str);

	for (int i = 0; i < contents.data_size(); ++i)
	{
		auto user = contents.data(i);
		int id = user.id(), x_off = user.xoff(), y_off = user.yoff();

		if (x_off == 0 && y_off == 0) {
			printf("몬스터와 전투중입니다. 움직일수 없습니다.\n");
			break;
		}

		Character* myChar;
		{
			Scoped_Rlock SR(&chars->srw);
			myChar = (chars->find(id));
		}
		myChar->setX(myChar->getX() + x_off);
		myChar->setY(myChar->getY() + y_off);

		printf("I am moved! My char id : %d, (%d,%d) \n", myChar->getID(), myChar->getX(), myChar->getY());
		//printf("my lv : %d, hp : %d, power : %d, exp : %d\n", myChar->getLv(), myChar->getPrtHp(), myChar->getPower(), myChar->getExp());
	}
	contents.clear_data();
}