#include <WinSock2.h>

#include "Scoped_Lock.h"
#include "cmap.h"
#include "mmap.h"

#include "../protobuf/monsterattackresult.pb.h"

void Handler_PMONSTER_ATTACK_RESULT(Character *myChar, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	MONSTER_ATTACK_RESULT::CONTENTS monsterattackresultContents;
	monsterattackresultContents.ParseFromString(*str);

	int id_m = monsterattackresultContents.id_m();
	int attackType = monsterattackresultContents.attacktype();

	for (int i = 0; i < monsterattackresultContents.data_size(); ++i)
	{
		auto monsterattackresult = monsterattackresultContents.data(i);
		int id = monsterattackresult.id();
		int prtHp = monsterattackresult.prthp();

		Character* targetChar = NULL;
		{
			Scoped_Rlock SR(&chars->srw);
			targetChar = chars->find(id);
		}
		if (targetChar == NULL){ printf("@@내가뜨면 안됨. 혹시뜸?\n"); }
		else
		{
			int prePrtHp;
			{
				Scoped_Wlock SW(targetChar->getLock());
				prePrtHp = targetChar->getPrtHp();
				targetChar->setPrtHp(prtHp);
			}
			if (prtHp == 0)
			{
				printf("유저 [ %d ] 가 죽었습니다.. 10초 뒤 리스폰 됩니다..\n", id);
			}
			else
			{
				int damage = prePrtHp - prtHp;
				printf("!!몬스터 [ %d ]가 %d 공격타입으로 유저 [ %d ] 에게 %d 만큼 피해를 입혔습니다.\n",
					id_m, attackType, targetChar->getID(), damage);
			}
		}
	}

}