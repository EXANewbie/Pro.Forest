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

	//for���� �� ���� ���� ������ �Ͽ��� ��츦 ����.
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
			printf("���߸� �ȵŴµ� ��?\n");
			exit(0);
		}
	
		if (*myID == id)//�����Ѱ��� ���϶�
		{
			if (targetMon->getPrtHp() == 0)
			{
				printf("- �ٱ�� %d�� ����Ͽ� ����%s[%d] (��)�� %d��ŭ �����Ͽ� �׿����ϴ�!!\n\n",
					attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
			
				mons->erase(targetMon->getID());	
			}
			else
			{
				printf("- ��� %d�� �̿��Ͽ� ���� %s[%d] (��)�� %d��ŭ �����Ͽ����ϴ�.\n\n",
					attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
			}
		}
		else
		{
			if (targetMon->getPrtHp() == 0)
			{
				printf("�� ������ %s���� ��� %d�� �̿��Ͽ� ����%s[%d]�� %d��ŭ �����Ͽ� �׿����ϴ�!!\n\n",
					atkChar->getName().c_str(), attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
				
				mons->erase(targetMon->getID());
			}
			else
			{
				printf("�� ���� %s���� ��� %d�� �̿��Ͽ� ���� %s[%d]�� %d��ŭ �����Ͽ����ϴ�.\n\n",
					atkChar->getName().c_str(), attckType, targetMon->getName().c_str(), targetMon->getID(), damage);
			}
		}
		
	}
}