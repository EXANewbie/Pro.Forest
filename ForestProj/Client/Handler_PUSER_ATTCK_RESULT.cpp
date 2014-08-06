#include <WinSock2.h>

#include "cmap.h"
#include "mmap.h"
#include "Scoped_Lock.h"

#include "../protobuf/userattackresult.pb.h"

void Handler_PUSER_ATTCK_RESULT(Character *myChar, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	USER_ATTACK_RESULT::CONTENTS userattackresultContents;
	userattackresultContents.ParseFromString(*str);

	//for문을 한 것은 다중 공격을 하였을 경우를 위해.
	for (int i = 0; i < userattackresultContents.data_size(); ++i)
	{
		auto userattackresult = userattackresultContents.data(i);
		int id = userattackresult.id();
		int attckType = userattackresult.attcktype();
		int id_m = userattackresult.id_m();
		int prtHp_m = userattackresult.prthp_m();

		Monster* targetMon;
		{
			Scoped_Rlock SR(&mons->srw);
			targetMon = mons->find(id_m);
		}
		int damage;
		{
			Scoped_Wlock SW(targetMon->getLock());
			damage = targetMon->getPrtHp() - prtHp_m;
			targetMon->setPrtHp(prtHp_m);
		}
		Character* atkChar = NULL;
		{
			Scoped_Rlock SR(&chars->srw);
			atkChar = chars->find(id);
		}
		if (atkChar == NULL)
		{
			printf("나뜨면 안돼는데 뜸?\n");
		}
	

		if (myChar->getID() == id)//공격한것이 나일때
		{
			if (targetMon->getPrtHp() == 0)
			{
				printf("- ☆기술 %d을 사용하여 몬스터%s[%d] (을)를 %d만큼 공격하여 죽였습니다!!\n\n",
					attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
				{
					Scoped_Wlock SW(&mons->srw);

					//몬스터에 대해서는 lock을 걸 필요가 없는듯 한데..
					mons->erase(targetMon->getID());
				}
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
				{
					Scoped_Wlock SW(&mons->srw);

					//몬스터에 대해서는 lock을 걸 필요가 없는듯 한데..
					mons->erase(targetMon->getID());
				}
			}
			else
			{
				//이름도 붙여주고 싶다. protobuf string 넘겨줘도 문제없게 하고싶음.  하였음
				printf("※ 유저 %s님이 기술 %d을 이용하여 몬스터 %s[%d]를 %d만큼 공격하였습니다.\n\n",
					atkChar->getName().c_str(), attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
			}
		}
		
	}
}