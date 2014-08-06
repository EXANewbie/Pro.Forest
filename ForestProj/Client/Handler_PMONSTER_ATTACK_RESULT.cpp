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

	Monster* atkMon = NULL;
	{
		Scoped_Rlock SR(&mons->srw);
		atkMon = mons->find(id_m);
	}

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
		
		if (targetChar == NULL){ printf("@@�����߸� �ȵ�. Ȥ�ö�?\n"); }
		else if (atkMon == NULL) { printf("���� �߸� �ȵʤ���\n"); }
		else
		{
			if (targetChar->getID() == myChar->getID())// ���϶�
			{
				int prePrtHp;
				{
					Scoped_Wlock SW(targetChar->getLock());
					prePrtHp = targetChar->getPrtHp();
					targetChar->setPrtHp(prtHp);
				}
				if (prtHp == 0)
				{
					int damage = prePrtHp - prtHp;
					printf("- ���� [ %s ]�� %d ����Ÿ������ %d ��ŭ ���ظ� �������ϴ�.\n",
						atkMon->getName().c_str(), attackType, damage);
					printf("- ü���� ��� �����Ǿ����ϴ�..\n����...�״ٴ�.. 10�� �� ������ �˴ϴ�..\n");
				}
				else
				{
					int damage = prePrtHp - prtHp;
					printf("- ���� [ %s ]�� %d ����Ÿ������ %d ��ŭ ���ظ� �������ϴ�.\n",
						atkMon->getName().c_str(), attackType, damage);
				}
			}
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
					int damage = prePrtHp - prtHp;
					printf("�� ���� %s���� ���� [ %s ]�� %d ����Ÿ������ %d ��ŭ ���ظ� �Ծ����ϴ�.\n",
						targetChar->getName(), atkMon->getName().c_str(), attackType, damage);
					printf("�� ���� %s�Բ��� ����ϼ̽��ϴ�.");
				}
				else
				{
					int damage = prePrtHp - prtHp;
					printf("�� ���� %s���� ���� [ %s ]�� %d ����Ÿ������ %d ��ŭ ���ظ� �Ծ����ϴ�.\n",
						targetChar->getName(), atkMon->getName().c_str(), attackType, damage);
				}
			}
			
		}
	}

}