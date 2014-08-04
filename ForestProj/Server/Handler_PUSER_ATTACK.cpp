#include <string>

#include "../protobuf/userattack.pb.h"
#include "../protobuf/userattackresult.pb.h"
#include "../protobuf/setuserlv.pb.h"

#include "Check_Map.h"
#include "Completion_Port.h"
#include "character.h"
#include "Sock_set.h"
#include "DMap.h"
#include "Scoped_Lock.h"
#include "DMap_monster.h"
#include "msg.h"

void send_message(msg, vector<Character *> &, bool);
void make_vector_id_in_room(E_List *, vector<Character*>&);

void Handler_PUSER_ATTCK(Character *myChar, std::string* str)
{
	F_Vector* FVEC = F_Vector::getInstance();
	F_Vector_Mon* FVEC_M = F_Vector_Mon::getInstance();
	USER_ATTACK_RESULT::CONTENTS userattackresultContents;
	SET_USER_LV::CONTENTS setuserlvContents;
	
	//****** 이부분 좀 고민좀 됨. 공격을 하고 바로 그자리에서 빠지면 브로드 캐스트가 어떻게 될까 등등. 일단 뒤로 미루겠음.
	E_List* elist;
	{
		Scoped_Rlock SR(myChar->getLock());
		elist = FVEC->get(myChar->getX(), myChar->getY());
	}
	vector<Character*> charId_in_room;
	{
		Scoped_Rlock SR(&elist->slock);
		make_vector_id_in_room(elist, charId_in_room);
	}
	// ~고민되는부분

	USER_ATTACK::CONTENTS userattackContents;
	userattackContents.ParseFromString(*str);

	bool existLvUser = false;
	//여러 몬스터을 공격할수도 있기에 for문으로 구성.
	for (int i = 0; i < userattackContents.data_size(); ++i)
	{
		auto userattack = userattackContents.data(i);
		
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
		int monprtHp;
		bool kill = false;
		{
			Scoped_Wlock SR(mon->getLock());
			mon->attacked(damage);
			monprtHp = mon->getPrtHp();
			if (monprtHp == 0)
			{
				//몬스터 시키 죽였따!!
				//mon->SET_DEAD_MODE();

				//경험치를 얻도록하자.
				kill = true;
			}
		}
		bool lvUp = false;
		if (kill == true)
		{
			Scoped_Wlock SW(myChar->getLock());
			int prtExp = myChar->getExp() + knightExp;
			int prtLv = myChar->getLv();
			if (prtExp >= maxExp[prtLv])
			{
				myChar->setLv(prtLv + 1, HpPw[prtLv][0], HpPw[prtLv][1]);
				myChar->setExp(prtExp - maxExp[prtLv]);
				lvUp = true;
			}
			else
			{
				myChar->setExp(prtExp);
			}
		}
		if (lvUp == true)
		{
			existLvUser = true;
			Scoped_Rlock SR(myChar->getLock());
			auto setuserlv = setuserlvContents.add_data();
			setuserlv->set_id(myChar->getID());
			setuserlv->set_lv(myChar->getLv());
			setuserlv->set_maxhp(myChar->getMaxHp());
			setuserlv->set_power(myChar->getPower());
			setuserlv->set_exp(myChar->getExp());
		}

		// 몬스터 객체가 공격 당하고나서의 현재 체력을 같은방의 유저들에게 알림.
		auto userattackresult = userattackresultContents.add_data();
		userattackresult->set_id(myChar->getID()); // ID는 변하지 않아서 리드락 걸 필요가 읍다!?
		userattackresult->set_attcktype(attckType);
		userattackresult->set_id_m(id_mon);
		userattackresult->set_prthp_m(monprtHp);
	}
	
	std::string bytestring;
	int len;
	userattackresultContents.SerializeToString(&bytestring);
	len = bytestring.length();
	send_message(msg(PUSER_ATTCK_RESULT, len, bytestring.c_str()), charId_in_room, false);
	bytestring.clear();

	if (existLvUser == true)
	{
		setuserlvContents.SerializeToString(&bytestring);
		len = bytestring.length();
		send_message(msg(PUSER_SET_LV, len, bytestring.c_str()), charId_in_room, false);
	}
}