#include <WinSock2.h>

#include "Scoped_Lock.h"
#include "cmap.h"
#include "mmap.h"

#include "../protobuf/moveuser.pb.h"

void Handler_PMOVE_USER(int *myID, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	MOVE_USER::CONTENTS contents;
	contents.ParseFromString(*str);

	Scoped_Wlock SW(&chars->srw);
	
	for (int i = 0; i < contents.data_size(); ++i)
	{
		auto user = contents.data(i);
		int id = user.id(), x_off = user.xoff(), y_off = user.yoff();

		if (x_off == 0 && y_off == 0) {
			printf("���Ϳ� �������Դϴ�. �����ϼ� �����ϴ�.\n");
			break;
		}

		Character* moveChar = chars->find(id); //����ܰ迡�� ������� myChar

		moveChar->setX(moveChar->getX() + x_off);
		moveChar->setY(moveChar->getY() + y_off);
		
		printf("(%d,%d)�� �̵��ߴ�! \n", moveChar->getX(), moveChar->getY());
	}
	contents.clear_data();
}