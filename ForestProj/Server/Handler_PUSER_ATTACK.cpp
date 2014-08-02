#include <string>

#include "../protobuf/userattack.pb.h"
#include "../protobuf/userattackresult.pb.h"

#include "Check_Map.h"
#include "Completion_Port.h"
#include "character.h"
#include "Sock_set.h"
#include "DMap.h"
#include "Scoped_Lock.h"
#include "DMap_monster.h"
#include "msg.h"


void Handler_PUSER_ATTCK(Character *myChar, std::string* str)
{
	F_Vector_Mon* FVEC_M = F_Vector_Mon::getInstance();
	
	USER_ATTACK::CONTENTS userattackContents;
	userattackContents.ParseFromString(*str);

	for (int i = 0; i < userattackContents.data_size(); ++i)
	{
		auto userattack = userattackContents.data(i);
		//int char_id = userattack.id(), x = userattack.x(), y = userattack.y();
		int attckType = userattack.attcktype();
		int mon_id = userattack.id_m(), mon_x = userattack.x_m(), mon_y = userattack.y_m();
		E_List_Mon* elist_m = FVEC_M->get(mon_x, mon_y);
		Monster* mon = NULL;
		
		// ���� ��ü�� ����.
		{
			Scoped_Rlock SR(&elist_m->slock);
			mon = elist_m->find(mon_id);
		}

		// ���������� ���.
		int damage = myChar->getPower();
		
		// ���� ��ü�� ������ ����.
		mon->attacked(damage);

		// ���� ��ü�� ������ ���� ���� �������� �˸�.

		USER_ATTACK_RESULT::CONTENTS userattackresultContents;


	}
}