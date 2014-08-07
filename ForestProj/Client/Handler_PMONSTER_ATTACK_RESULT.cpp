#include <WinSock2.h>

#include "Scoped_Lock.h"
#include "cmap.h"
#include "mmap.h"

#include "../protobuf/monsterattackresult.pb.h"

void Handler_PMONSTER_ATTACK_RESULT(int *myID, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	MONSTER_ATTACK_RESULT::CONTENTS monsterattackresultContents;
	monsterattackresultContents.ParseFromString(*str);

	Scoped_Wlock SW1(&chars->srw);
	Scoped_Wlock SW2(&mons->srw);
	
	int id_m = monsterattackresultContents.id_m();
	int attackType = monsterattackresultContents.attacktype();

	Monster* atkMon = mons->find(id_m);
	
	for (int i = 0; i < monsterattackresultContents.data_size(); ++i)
	{
		auto monsterattackresult = monsterattackresultContents.data(i);
		int id = monsterattackresult.id();
		int prtHp = monsterattackresult.prthp();

		Character* targetChar = chars->find(id);
		
		if (targetChar == NULL){ printf("@@내가뜨면 안됨. 혹시뜸?\n"); exit(0); }
		else if (atkMon == NULL) { printf("내가 뜨면 안됨ㅇㅇ\n"); exit(0); }
		else
		{
			if (targetChar->getID() == *myID)// 나일때
			{
				int prePrtHp = targetChar->getPrtHp();
				targetChar->setPrtHp(prtHp);
				
				if (prtHp == 0)
				{
					int damage = prePrtHp - prtHp;
					printf("- 몬스터 [ %s(%d) ]가 %d 공격타입으로 %d 만큼 피해를 입혔습니다.\n",
						atkMon->getName().c_str(), atkMon->getID(), attackType, damage);
					printf("- 체력이 모두 소진되었습니다..\n내가...죽다니.. 10초 뒤 리스폰 됩니다..\n");
					chars->clear();
					mons->clear();
					break;
				}
				else
				{
					int damage = prePrtHp - prtHp;
					printf("- 몬스터 [ %s(%d) ]가 %d 공격타입으로 %d 만큼 피해를 입혔습니다.\n",
						atkMon->getName().c_str(), atkMon->getID(), attackType, damage);
				}
			}
			else
			{
				int	prePrtHp = targetChar->getPrtHp();
				targetChar->setPrtHp(prtHp);
				
				int damage = prePrtHp - prtHp;
				printf("※ 유저 %s님이 몬스터 [ %s(%d) ]의 %d 공격타입으로 %d 만큼 피해를 입었습니다.\n",
					targetChar->getName().c_str(), atkMon->getName().c_str(), atkMon->getID(), attackType, damage);

				if (prtHp == 0)
				{
					printf("※ 유저 %s님께서 사망하셨습니다.\n", targetChar->getName().c_str());
		
					chars->erase(targetChar->getID());
				}
			}
		}
	}
}