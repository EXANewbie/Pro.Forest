#include <WinSock2.h>

#include "cmap.h"
#include "mmap.h"
#include "Scoped_Lock.h"

#include "../protobuf/setuserexp.pb.h"

void Handler_PUSER_SET_EXP(int *myID, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	SET_USER_EXP::CONTENTS setuserexpContents;
	setuserexpContents.ParseFromString(*str);

	Scoped_Wlock SW(&mons->srw);
	auto setuserexp = setuserexpContents.data(0);

	int id = setuserexp.id();
	int expUp = setuserexp.expup();

	Character* expUpChar = chars->find(id);
		
	expUpChar->setExpUp(expUp);
	
	if (*myID == id)// �����
	{
		printf("- ����ġ�� %d ����߽��ϴ�!!\n", expUp);
	}
	else
	{
		printf("�� ���� %s���� ����ġ�� %d ����߽��ϴ�!!\n", expUpChar->getName().c_str(), expUp);
	}
	
}