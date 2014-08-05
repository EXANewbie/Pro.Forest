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

	//for���� �� ���� ���� ������ �Ͽ��� ��츦 ����.
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

		if (myChar->getID() == id)//�����Ѱ��� ���϶�
		{
			if (targetMon->getPrtHp() == 0)
			{
				printf("- �ٱ�� %d�� ����Ͽ� ����%s[%d] (��)�� %d��ŭ �����Ͽ� �׿����ϴ�!!\n",
					attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
				{
					Scoped_Wlock SW(&mons->srw);

					//���Ϳ� ���ؼ��� lock�� �� �ʿ䰡 ���µ� �ѵ�..
					mons->erase(targetMon->getID());
				}
			}
			else
			{
				printf("- ��� %d�� �̿��Ͽ� ���� %s[%d] (��)�� %d��ŭ �����Ͽ����ϴ�.\n",
					attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
			}
		}
		else
		{
			if (targetMon->getPrtHp() == 0)
			{
				printf("�� ������ %s���� ��� %d�� �̿��Ͽ� ����%s[%d]�� %d��ŭ �����Ͽ� �׿����ϴ�!!\n",
					myChar->getName().c_str(), attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
				{
					Scoped_Wlock SW(&mons->srw);

					//���Ϳ� ���ؼ��� lock�� �� �ʿ䰡 ���µ� �ѵ�..
					mons->erase(targetMon->getID());
				}
			}
			else
			{
				//�̸��� �ٿ��ְ� �ʹ�. protobuf string �Ѱ��൵ �������� �ϰ����.  �Ͽ���
				printf("�� ���� %s���� ��� %d�� �̿��Ͽ� ���� %s[%d]�� %d��ŭ �����Ͽ����ϴ�.\n",
					myChar->getName().c_str(), attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
			}
		}
		
	}
}