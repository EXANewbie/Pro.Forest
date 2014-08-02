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

void send_message(msg, vector<Character *> &, bool);

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
		int id_mon = userattack.id_m(), x_mon = userattack.x_m(), y_mon = userattack.y_m();
		E_List_Mon* elist_m = FVEC_M->get(x_mon, y_mon);
		Monster* mon = NULL;
		
		// 몬스터 객체를 얻어옴.
		{
			Scoped_Rlock SR(&elist_m->slock);
			mon = elist_m->find(id_mon);
		}

		// 유저데미지 계산.
		int damage;
		{
			Scoped_Rlock SR(myChar->getLock());
			damage = myChar->getPower();
		}
		// 몬스터 객체에 데미지 입힘.
		{
			Scoped_Rlock SR(mon->getLock());
			mon->attacked(damage);
		}
		// 몬스터 객체가 데미지 당한 량을 유저에게 알림.

		USER_ATTACK_RESULT::CONTENTS userattackresultContents;
		auto userattackresult = userattackresultContents.add_data();
		userattackresult->set_attcktype(attckType);
		userattackresult->set_id_m(id_mon);
		userattackresult->set_x_m(x_mon);
		userattackresult->set_y_m(y_mon);
		userattackresult->set_damage(damage);

		std::string bytestring;
		userattackresult->SerializeToString(&bytestring);
		int len = bytestring.length();
		vector<Character*> me;
		me.push_back(myChar);

		send_message(msg(PUSER_ATTCK_RESULT, len, bytestring.c_str()), me, false);

	}
}