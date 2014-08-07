#include <WinSock2.h>

#include "Scoped_Lock.h"
#include "cmap.h"
#include "mmap.h"

#include "../protobuf/erasemonster.pb.h"

void Handler_PERASE_MON(int *myID, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	ERASE_MONSTER::CONTENTS erasemonsterContents;
	erasemonsterContents.ParseFromString(*str);

	Scoped_Wlock SW(&mons->srw);
	for (int i = 0; i<erasemonsterContents.data_size(); ++i)
	{
		auto mon = erasemonsterContents.data(i);
		int id = mon.id();
		std::string monName;
		
		monName = mons->find(id)->getName();
		mons->erase(id);
		
		printf("몬스터[%s]를 지나쳤다! : %d \n", monName.c_str(), id);
	}
	erasemonsterContents.clear_data();
}