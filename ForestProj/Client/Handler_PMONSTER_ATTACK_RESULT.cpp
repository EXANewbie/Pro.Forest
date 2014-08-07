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
		
		if (targetChar == NULL){ printf("@@�����߸� �ȵ�. Ȥ�ö�?\n"); exit(0); }
		else if (atkMon == NULL) { printf("���� �߸� �ȵʤ���\n"); exit(0); }
		else
		{
			if (targetChar->getID() == *myID)// ���϶�
			{
				int prePrtHp = targetChar->getPrtHp();
				targetChar->setPrtHp(prtHp);
				
				if (prtHp == 0)
				{
					int damage = prePrtHp - prtHp;
					printf("- ���� [ %s(%d) ]�� %d ����Ÿ������ %d ��ŭ ���ظ� �������ϴ�.\n",
						atkMon->getName().c_str(), atkMon->getID(), attackType, damage);
					printf("- ü���� ��� �����Ǿ����ϴ�..\n����...�״ٴ�.. 10�� �� ������ �˴ϴ�..\n");
					chars->clear();
					mons->clear();
					break;
				}
				else
				{
					int damage = prePrtHp - prtHp;
					printf("- ���� [ %s(%d) ]�� %d ����Ÿ������ %d ��ŭ ���ظ� �������ϴ�.\n",
						atkMon->getName().c_str(), atkMon->getID(), attackType, damage);
				}
			}
			else
			{
				int	prePrtHp = targetChar->getPrtHp();
				targetChar->setPrtHp(prtHp);
				
				int damage = prePrtHp - prtHp;
				printf("�� ���� %s���� ���� [ %s(%d) ]�� %d ����Ÿ������ %d ��ŭ ���ظ� �Ծ����ϴ�.\n",
					targetChar->getName().c_str(), atkMon->getName().c_str(), atkMon->getID(), attackType, damage);

				if (prtHp == 0)
				{
					printf("�� ���� %s�Բ��� ����ϼ̽��ϴ�.\n", targetChar->getName().c_str());
		
					chars->erase(targetChar->getID());
				}
			}
		}
	}
}