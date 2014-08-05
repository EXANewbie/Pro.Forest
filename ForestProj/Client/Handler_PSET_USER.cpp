#include <WinSock2.h>

#include "Scoped_Lock.h"
#include "cmap.h"
#include "mmap.h"

#include "../protobuf/setuser.pb.h"

void Handler_PSET_USER(Character *myChar, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	SET_USER::CONTENTS contents;
	contents.ParseFromString(*str);
	for (int i = 0; i<contents.data_size(); ++i)
	{
		auto user = contents.data(i);
		Character* other = new Character;
		int id = user.id(), x = user.x(), y = user.y();
		int lv = user.lv(), maxHp = user.maxhp(), power = user.power(), prtExp = user.prtexp(), maxExp = user.maxexp();
		other->setID(id);
		other->setX(x);
		other->setY(y);
		other->setLv(lv, maxHp, power, maxExp);
		other->setPrtExp(prtExp);
		{
			Scoped_Wlock SW(&chars->srw);
			chars->insert(id, other);
		}
		printf("동료[ %d ]가 왔습니다. ", other->getID());
		printf("레벨 : %d, 체력 : %d, 공격력 : %d, 경혐치 : %d\n", other->getLv(), other->getPrtHp(), other->getPower(), other->getPrtExp());
	}
	contents.clear_data();

}