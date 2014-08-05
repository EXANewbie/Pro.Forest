#include <WinSock2.h>

#include "cmap.h"
#include "mmap.h"
#include "Scoped_Lock.h"

#include "../protobuf/setuserlv.pb.h"

void Handler_PUSER_SET_LV(Character *myChar, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	SET_USER_LV::CONTENTS setuserlvContents;
	setuserlvContents.ParseFromString(*str);

	{
		Scoped_Rlock SR(&chars->srw);
		for (int i = 0; i < setuserlvContents.data_size(); ++i)
		{
			auto setuserlv = setuserlvContents.data(i);
			int id = setuserlv.id();
			int lv = setuserlv.lv();
			int maxHp = setuserlv.maxhp();
			int power = setuserlv.power();
			int expUp = setuserlv.expup();
			int maxexp = setuserlv.maxexp();

			Character* lvUpChar = chars->find(id);
			if (lvUpChar == NULL) printf("나 나오면 안돼는데 나옴?");
			else
			{
				Scoped_Wlock SW(lvUpChar->getLock());
				lvUpChar->setExpUp(expUp);
				lvUpChar->setLv(lv, maxHp, power, maxexp);
				printf("유저 [%d]가 레벨이 %d로 올랐습니다!!\n", id, lv);
			}
		}
	}
}