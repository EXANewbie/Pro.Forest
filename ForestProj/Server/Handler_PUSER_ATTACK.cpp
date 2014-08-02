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
		
		// 몬스터 객체를 얻어옴.
		{
			Scoped_Rlock SR(&elist_m->slock);
			mon = elist_m->find(mon_id);
		}

		// 유저데미지 계산.
		int damage = myChar->getPower();
		
		// 몬스터 객체에 데미지 입힘.
		mon->attacked(damage);

		// 몬스터 객체가 데미지 당한 량을 유저에게 알림.

		USER_ATTACK_RESULT::CONTENTS userattackresultContents;


	}
}