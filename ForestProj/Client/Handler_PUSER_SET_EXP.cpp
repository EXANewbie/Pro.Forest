#include <WinSock2.h>

#include "cmap.h"
#include "mmap.h"
#include "Scoped_Lock.h"

#include "../protobuf/setuserexp.pb.h"

void Handler_PUSER_SET_EXP(Character *myChar, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	SET_USER_EXP::CONTENTS setuserexpContents;
	setuserexpContents.ParseFromString(*str);
	auto setuserexp = setuserexpContents.data(0);

	int id = setuserexp.id();
	int expUp = setuserexp.expup();

	Character* expUpChar = chars->find(id);
	{
		Scoped_Wlock Sw(expUpChar->getLock());
		expUpChar->setExpUp(expUp);
	}
	printf("유저 [%d]가 경험치가 상승했습니다!!\n", expUpChar->getID());
}