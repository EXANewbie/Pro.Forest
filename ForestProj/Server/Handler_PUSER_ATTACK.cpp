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
	
	//****** �̺κ� �� ����� ��. ������ �ϰ� �ٷ� ���ڸ����� ������ ��ε� ĳ��Ʈ�� ��� �ɱ� ���. �ϴ� �ڷ� �̷����.
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
	// ~��εǴºκ�

	USER_ATTACK::CONTENTS userattackContents;
	userattackContents.ParseFromString(*str);

	bool existKillUser = false;
	//���� ������ �����Ҽ��� �ֱ⿡ for������ ����.
	for (int i = 0; i < userattackContents.data_size(); ++i)
	{
		auto userattack = userattackContents.data(i);
		
		int attckType = userattack.attcktype();
		int id_mon = userattack.id_m(), x_mon = userattack.x_m(), y_mon = userattack.y_m();
		E_List_Mon* elist_m = FVEC_M->get(x_mon, y_mon);
		Monster* mon = NULL;
		
		// ���� ��ü�� ����.
		{
			Scoped_Rlock SR(&elist_m->slock);
			mon = elist_m->find(id_mon);
		}

		// ���������� ���.
		int damage;
		{
			Scoped_Rlock SR(myChar->getLock());
			damage = myChar->getPower();
		}

		// ���� ��ü�� ������ ����.
		int monprtHp;
		bool kill = false;
		{
			Scoped_Wlock SR(mon->getLock());
			mon->attacked(damage);
			monprtHp = mon->getPrtHp();
			if (monprtHp == 0)
			{
				//���� ��Ű �׿���!!
				//mon->SET_DEAD_MODE();
				{
					Scoped_Wlock SW(&elist_m->slock);
					elist_m->erase(mon->getID());
				}
				//����ġ�� �򵵷�����.
				kill = true;
			}
		}
		bool lvUp = false;
		if (kill == true)
		{
			Scoped_Wlock SW(myChar->getLock());
			int prtExp = myChar->getExp() + knightExp;
			int prtLv = myChar->getLv();
			if (prtExp >= maxExp[prtLv-1])
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
			existKillUser = true;
			Scoped_Rlock SR(myChar->getLock());
			auto setuserlv = setuserlvContents.add_data();
			setuserlv->set_id(myChar->getID());
			setuserlv->set_lv(myChar->getLv());
			setuserlv->set_maxhp(myChar->getMaxHp());
			setuserlv->set_power(myChar->getPower());
			setuserlv->set_exp(myChar->getExp());
		}

		// ���� ��ü�� ���� ���ϰ����� ���� ü���� �������� �����鿡�� �˸�.
		auto userattackresult = userattackresultContents.add_data();
		userattackresult->set_id(myChar->getID()); // ID�� ������ �ʾƼ� ����� �� �ʿ䰡 ����!?
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

	if (existKillUser == true)
	{
		setuserlvContents.SerializeToString(&bytestring);
		len = bytestring.length();
		send_message(msg(PUSER_SET_LV, len, bytestring.c_str()), charId_in_room, false);
	}
}