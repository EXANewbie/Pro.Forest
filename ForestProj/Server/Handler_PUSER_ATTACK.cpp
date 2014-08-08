#include <string>

#include "../protobuf/userattack.pb.h"
#include "../protobuf/userattackresult.pb.h"
#include "../protobuf/setuserlv.pb.h"
#include "../protobuf/setuserexp.pb.h"

#include "Check_Map.h"
#include "Completion_Port.h"
#include "character.h"
#include "Sock_set.h"
#include "DMap.h"
#include "Scoped_Lock.h"
#include "DMap_monster.h"
#include "msg.h"
#include "types.h"

void send_message(msg, vector<Character *> &, bool);
void make_vector_id_in_room(E_List *, vector<Character*>&);

void Handler_PUSER_ATTCK(Character *myChar, std::string* str)
{
	// ���� ĳ���Ͱ� �����ϴ� �Ϳ� ���ؼ� ��ȿó��.
	F_Vector* FVEC = F_Vector::getInstance();
	F_Vector_Mon* FVEC_M = F_Vector_Mon::getInstance();
	USER_ATTACK::CONTENTS userattackContents;
	USER_ATTACK_RESULT::CONTENTS userattackresultContents;
	SET_USER_LV::CONTENTS setuserlvContents;
	SET_USER_EXP::CONTENTS setuserexpContents;
	vector<Character*> me;

	//****** �̺κ� �� ����� ��. ������ �ϰ� �ٷ� ���ڸ����� ������ ��ε� ĳ��Ʈ�� ��� �ɱ� ���. �ϴ� �ڷ� �̷����.
	E_List* elist;
	E_List_Mon* elist_m;
	vector<Character*> charId_in_room;

	me.push_back(myChar);

	if (myChar->getPrtHp() == 0)
	{
		return;
	}

	{
		Scoped_Rlock CHARACTER_READ_LOCK(myChar->getLock());
		int x, y;
		x = myChar->getX();
		y = myChar->getY();
		elist = FVEC->get(x, y);
		elist_m = FVEC_M->get(x, y);
	}

	{
		Scoped_Wlock E_LIST_WRITE_LOCK(&elist->slock);
		Scoped_Wlock E_LIST_MON_WRITE_LOCK(&elist_m->slock);

		if (myChar->getPrtHp() == 0)
		{
			return;
		}

		make_vector_id_in_room(elist, charId_in_room);

		userattackContents.ParseFromString(*str);

		bool existKillUser, existLvUpUser;

		for (int i = 0; i < userattackContents.data_size(); ++i)
		{
			existKillUser = false;
			existLvUpUser = false;
			auto userattack = userattackContents.data(i);

			int attckType = userattack.attcktype();
			int id_mon = userattack.id_m(), x_mon = userattack.x_m(), y_mon = userattack.y_m();
			Monster* mon = NULL;

			// ���� ��ü�� ����.

			mon = elist_m->find(id_mon);

			// �̹� ���� ���Ϳ� ���ؼ� ���ݽ�ȣ�� �޾������� ����ó��.
			if (mon == NULL)
			{
				// Ŭ���̾�Ʈ�� ���Ͱ� ����������, �������� ���� ���ŵ� �����̱� ������, ��ٸ��� ���Ͱ� ���ŵǴ� ��Ȳ
				return;
			}

			// ���������� ���.
			int damage;
			{
				Scoped_Rlock CHARACTER_READ_LOCK(myChar->getLock());
				damage = myChar->getPower();
			}

			// ���� ��ü�� ������ ����.
			int monprtHp, expUp = 0;
			bool kill = false;
			{
				Scoped_Wlock MONSTER_WRITE_LOCK(mon->getLock());
				mon->attacked(damage);
				monprtHp = mon->getPrtHp();
				if (monprtHp == 0)
				{
					//���� ��Ű �׿���!!
					expUp = mon->getExp();
					elist_m->erase(mon);
					
					//����ġ�� �򵵷�����.
					mon->SET_DEAD_MODE();
					kill = true;
				}
			}
			bool lvUp = false;
			if (kill == true)
			{
				int prtLv;
				{
					Scoped_Wlock CHARACTER_WRITE_LOCK(myChar->getLock());
					prtLv = myChar->getLv();
					myChar->setExpUp(expUp);
				}

				if (myChar->getPrtExp() >= myChar->getMaxExp())
				{
					prtLv++;
					myChar->setLv(prtLv, HpPw[prtLv][0], HpPw[prtLv][1], maxExp[prtLv]);
					lvUp = true;
				}

				if (lvUp == true)
				{
					existLvUpUser = true;
					Scoped_Rlock CHARACTER_READ_LOCK(myChar->getLock());
					auto setuserlv = setuserlvContents.add_data();
					setuserlv->set_id(myChar->getID());
					setuserlv->set_lv(myChar->getLv());
					setuserlv->set_maxhp(myChar->getMaxHp());
					setuserlv->set_power(myChar->getPower());
					setuserlv->set_expup(expUp);
					setuserlv->set_maxexp(myChar->getMaxExp());
				}
				else
				{
					existKillUser = true;
					Scoped_Rlock CHARACTER_READ_LOCK(myChar->getLock());
					auto setuserexp = setuserexpContents.add_data();
					setuserexp->set_id(myChar->getID());
					setuserexp->set_expup(expUp);
				}
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

		if (existLvUpUser == true)
		{
			setuserlvContents.SerializeToString(&bytestring);
			len = bytestring.length();
			send_message(msg(PUSER_SET_LV, len, bytestring.c_str()), charId_in_room, false);
			bytestring.clear();
		}
		if (existKillUser == true)
		{
			setuserexpContents.SerializeToString(&bytestring);
			len = bytestring.length();
			send_message(msg(PUSER_SET_EXP, len, bytestring.c_str()), charId_in_room, false);
			bytestring.clear();
		}
	}
	//���� ������ �����Ҽ��� �ֱ⿡ for������ ����.
}