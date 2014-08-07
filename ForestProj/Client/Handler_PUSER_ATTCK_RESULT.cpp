#include <WinSock2.h>

#include "cmap.h"
#include "mmap.h"
#include "Scoped_Lock.h"

#include "../protobuf/userattackresult.pb.h"

void Handler_PUSER_ATTCK_RESULT(int *myID, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	USER_ATTACK_RESULT::CONTENTS userattackresultContents;
	userattackresultContents.ParseFromString(*str);

	Scoped_Wlock SW1(&chars->srw);
	Scoped_Wlock SW2(&mons->srw);

	//for문을 한 것은 다중 공격을 하였을 경우를 위해.
	for (int i = 0; i < userattackresultContents.data_size(); ++i)
	{
		auto userattackresult = userattackresultContents.data(i);
		int id = userattackresult.id();
		int attckType = userattackresult.attcktype();
		int id_m = userattackresult.id_m();
		int prtHp_m = userattackresult.prthp_m();

		Monster* targetMon = mons->find(id_m);
				
		int damage = targetMon->getPrtHp() - prtHp_m;
		targetMon->setPrtHp(prtHp_m);
		
		Character* atkChar = chars->find(id);
				
		if (atkChar == NULL)
		{
			printf("나뜨면 안돼는데 뜸?\n");
			exit(0);
		}
	
		if (*myID == id)//공격한것이 나일때
		{
			if (targetMon->getPrtHp() == 0)
			{
				printf("- ☆기술 %d을 사용하여 몬스터%s[%d] (을)를 %d만큼 공격하여 죽였습니다!!\n\n",
					attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
			
				mons->erase(targetMon->getID());	
			}
			else
			{
				printf("- 기술 %d을 이용하여 몬스터 %s[%d] (을)를 %d만큼 공격하였습니다.\n\n",
					attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
			}
		}
		else
		{
			if (targetMon->getPrtHp() == 0)
			{
				printf("※ ☆유저 %s님이 기술 %d을 이용하여 몬스터%s[%d]를 %d만큼 공격하여 죽였습니다!!\n\n",
					atkChar->getName().c_str(), attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
				
				mons->erase(targetMon->getID());
			}
			else
			{
				printf("※ 유저 %s님이 기술 %d을 이용하여 몬스터 %s[%d]를 %d만큼 공격하였습니다.\n\n",
					atkChar->getName().c_str(), attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
			}
		}
		
	}
}