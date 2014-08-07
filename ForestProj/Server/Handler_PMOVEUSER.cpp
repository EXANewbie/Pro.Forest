#include <string>

#include "../protobuf/moveuser.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/erasemonster.pb.h"
#include "../protobuf/setmonster.pb.h"

#include "character.h"
#include "DMap.h"
#include "Scoped_Lock.h"
#include "DMap_monster.h"
#include "msg.h"
/*
#include "Check_Map.h"
#include "types.h"
#include "Completion_Port.h"
#include "character.h"
#include "Sock_set.h"
#include "DMap.h"
#include "Scoped_Lock.h"

#include "Memory_Pool.h"
#include "monster.h"
#include "DMap_monster.h"
#include "msg.h"
*/

bool Boundary_Check(int, const int, const int, int, int);
void send_message(msg, vector<Character *> &, bool);
void make_vector_id_in_room_except_me(Character*, vector<Character *>&, bool);
void make_monster_vector_in_room(Character* myChar, vector<Monster *>& send_list, bool autolocked);
void printLog(const char *msg, ...);

using std::string;

void Handler_PMOVE_USER(Character *pCharacter, string* readContents)
{
	MOVE_USER::CONTENTS moveuserContents;
	ERASE_USER::CONTENTS eraseuserContents;
	SET_USER::CONTENTS setuserContents;
	ERASE_MONSTER::CONTENTS erasemonsterContents;
	SET_MONSTER::CONTENTS setmonsterContents;

	std::string bytestring;
	int len;

	auto Amap = Access_Map::getInstance();
	auto FVEC = F_Vector::getInstance();
	auto FVEC_M = F_Vector_Mon::getInstance();

	vector<Character*> charId_in_room_except_me;
	vector<Character *> me;
	me.push_back(pCharacter);
	vector<Monster *> vec_mon;

	moveuserContents.ParseFromString(*readContents);

	int cur_id, x_off, y_off;

	auto user = moveuserContents.data(0);
	cur_id = user.id();
	x_off = user.xoff();
	y_off = user.yoff();
	moveuserContents.clear_data();

	E_List *now_elist, *next_elist;
	E_List_Mon *now_elist_mon, *next_elist_mon;

	{
		Scoped_Rlock CHARACTER_READ_LOCK(pCharacter->getLock());
		int x = pCharacter->getX(), y = pCharacter->getY();

		if (pCharacter->getPrtHp() == 0)
		{
			//���� ���¿��� move ����� ��ȿ���� �ʴ�. ���� return;
			return;
		}
		
		if (Boundary_Check(cur_id, x, y, x_off, y_off) == false)
		{
			return;
		}

		int next_x = x + x_off;
		int next_y = y + y_off;

		now_elist = FVEC->get(x, y);
		now_elist_mon = FVEC_M->get(x, y);

		next_elist = FVEC->get(next_x, next_y);
		next_elist_mon = FVEC_M->get(next_x, next_y);
	}

	/* ��谪 üũ ���� */

	// ���� ���� �ִ� �濡 ���Ͱ� �ִ��� Ȯ���� �ϵ��� ����. ���Ϳ� �Բ������� �������̰� �� ���̱� ����.
	{
		Scoped_Wlock NOW_E_LIST_WRITE_LOCK(&now_elist->slock);
		Scoped_Wlock NOW_E_LIST_MON_WRITE_LOCK(&now_elist_mon->slock);

		Scoped_Wlock NEXT_E_LIST_WRITE_LOCK(&next_elist->slock);
		Scoped_Wlock NEXT_E_LIST_MON_WRITE_LOCK(&next_elist_mon->slock);

		if (pCharacter->getPrtHp() == 0)
		{
			//���� ���¿��� move ����� ��ȿ���� �ʴ�. ���� return;
			return;
		}

		if (!now_elist_mon->empty())
		{
			//���Ͱ� �ִµ�!! �� �����̷���!! ��!!?
			auto moveuser = moveuserContents.add_data();
			moveuser->set_id(cur_id);
			moveuser->set_xoff(0);
			moveuser->set_yoff(0);

			moveuserContents.SerializeToString(&bytestring);
			len = bytestring.size();

			send_message(msg(PMOVE_USER, len, bytestring.c_str()), me, true);

			eraseuserContents.clear_data();
			bytestring.clear();
			return;
		}

		make_vector_id_in_room_except_me(pCharacter, charId_in_room_except_me, false/*autolock*/);

		for (int i = 0; i < charId_in_room_except_me.size(); ++i)
		{
			auto eraseuser = eraseuserContents.add_data();
			eraseuser->set_id(charId_in_room_except_me[i]->getID());

			if (eraseuserContents.data_size() == ERASE_USER_MAXIMUM) // ERASE_USER_MAXIMUM�� �Ѱ�ġ�� �����Ϸ��� �� ��
			{
				eraseuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				send_message(msg(PERASE_USER, len, bytestring.c_str()), me, true);

				eraseuserContents.clear_data();
				bytestring.clear();
			}
		}

		eraseuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		// ���� ���� �������� ������ �����.
		send_message(msg(PERASE_USER, len, bytestring.c_str()), me, true);

		bytestring.clear();
		eraseuserContents.clear_data();

		// ���� ���� �����鿡�� ���� ������� �˸�
		auto eraseuser = eraseuserContents.add_data();
		eraseuser->set_id(cur_id);
		eraseuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PERASE_USER, len, bytestring.c_str()), charId_in_room_except_me, true);

		eraseuserContents.clear_data();

		make_monster_vector_in_room(pCharacter, vec_mon, false);

		for (int i = 0; i < vec_mon.size(); ++i)
		{
			auto erasemon = erasemonsterContents.add_data();
			erasemon->set_id(vec_mon[i]->getID());
		}
		erasemonsterContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PERASE_MON, len, bytestring.c_str()), me, true);

		erasemonsterContents.clear_data();
		bytestring.clear();
	}

	// ������ ���� �������� ������ ������

	// ���� �����濡 �ִ� ģ������ ����?
	{
		// ���� ���� �����鿡�� ���� ������� �˸�
		auto eraseuser = eraseuserContents.add_data();
		eraseuser->set_id(cur_id);
		eraseuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PERASE_USER, len, bytestring.c_str()), charId_in_room_except_me, true);

		// ���� ���� ���͵��� ������ �����.
		E_List_Mon* elist_m = FVEC_M->get(x, y);

//		Scoped_Rlock SRM(&elist_m->slock);

		make_monster_vector_in_room(pCharacter, vec_mon, false);

		for (int i = 0; i < vec_mon.size(); ++i)
		{
			auto erasemon = erasemonsterContents.add_data();
			erasemon->set_id(vec_mon[i]->getID());
		}
		erasemonsterContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PERASE_MON, len, bytestring.c_str()), me, true);

		bytestring.clear();
		erasemonsterContents.clear_data();
	}

	charId_in_room_except_me.clear();
	vec_mon.clear();

	// ĳ���͸� �ش� ��ǥ��ŭ �̵���Ŵ
	{
//		Scoped_Wlock SW(&elist->slock);
		elist->erase(cur_id);
	}
	int newX = x + x_off, newY = y + y_off;
	{
//		Scoped_Wlock SWU(pCharacter->getLock());
		pCharacter->setX(newX);
		pCharacter->setY(newY);
	}
	elist = FVEC->get(newX, newY);
	{
//		Scoped_Wlock SW(&elist->slock);
		elist->push_back(pCharacter);
	}

	//������ ���� �������� ����.
	{
		auto moveuser = moveuserContents.add_data();
		moveuser->set_id(cur_id);
		moveuser->set_xoff(x_off);
		moveuser->set_yoff(y_off);
	}
	moveuserContents.SerializeToString(&bytestring);
	len = bytestring.length();

	send_message(msg(PMOVE_USER, len, bytestring.c_str()), me, false);

	bytestring.clear();
	moveuserContents.clear_data();

	// ���� �����濡 �ִ� ģ������ ����?
	{
//		Scoped_Rlock SR(&elist->slock);
//		Scoped_Rlock SRU(pCharacter->getLock());
		make_vector_id_in_room_except_me(pCharacter, charId_in_room_except_me, false/*autolock*/);

		// ���ο� ���� �����鿡�� ���� �������� �˸�
		x = pCharacter->getX(), y = pCharacter->getY();
		int lv = pCharacter->getLv();
		int prtHp = pCharacter->getPrtHp();
		int maxHp = pCharacter->getMaxHp();
		int power = pCharacter->getPower();
		int prtExp = pCharacter->getPrtExp();
		int maxExp = pCharacter->getMaxExp();
		std::string name = pCharacter->getName();

		auto setuser = setuserContents.add_data();
		setuser->set_id(cur_id);
		setuser->set_x(x);
		setuser->set_y(y);
		setuser->set_name(name);
		setuser->set_lv(lv);
		setuser->set_prthp(prtHp);
		setuser->set_maxhp(maxHp);
		setuser->set_power(power);
		setuser->set_prtexp(prtExp);
		setuser->set_maxexp(maxExp);

		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_USER, len, bytestring.c_str()), charId_in_room_except_me, false);

		bytestring.clear();
		setuserContents.clear_data();

		for (int i = 0; i < charId_in_room_except_me.size(); ++i)
		{
			auto setuser = setuserContents.add_data();
			auto tmpChar = charId_in_room_except_me[i];
			setuser->set_id(tmpChar->getID());
			setuser->set_x(tmpChar->getX());
			setuser->set_y(tmpChar->getY());
			setuser->set_name(tmpChar->getName());
			setuser->set_lv(tmpChar->getLv());
			setuser->set_prthp(tmpChar->getPrtHp());
			setuser->set_maxhp(tmpChar->getMaxHp());
			setuser->set_power(tmpChar->getPower());
			setuser->set_prtexp(tmpChar->getPrtExp());
			setuser->set_maxexp(tmpChar->getMaxExp());

			if (setuserContents.data_size() == SET_USER_MAXIMUM) {
				setuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				send_message(msg(PSET_USER, len, bytestring.c_str()), me, true);

				setuserContents.clear_data();
				bytestring.clear();
			}
		}

		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_USER, len, bytestring.c_str()), me, true);

		bytestring.clear();
		setuserContents.clear_data();

		// ���ο� ���� ���͵��� ������ �����´�.
		E_List_Mon* elist_m = FVEC_M->get(x, y);
//		Scoped_Rlock SR_M(&elist_m->slock);
		make_monster_vector_in_room(pCharacter, vec_mon, false);

		for (int i = 0; i < vec_mon.size(); ++i)
		{
			Monster* tmpMon = vec_mon[i];
			{
				//Scoped_Wlock SW(tmpMon->getLock());
				tmpMon->SET_BATTLE_MODE();
			}

			auto setmon = setmonsterContents.add_data();
			setmon->set_id(tmpMon->getID());
			setmon->set_x(tmpMon->getX());
			setmon->set_y(tmpMon->getY());
			setmon->set_name(tmpMon->getName());
			setmon->set_lv(tmpMon->getLv());
			setmon->set_prthp(tmpMon->getPrtHp());
			setmon->set_maxhp(tmpMon->getMaxHp());
			setmon->set_power(tmpMon->getPower());

			if (setmonsterContents.data_size() == SET_MONSTER_MAXIMUM)
			{
				setmonsterContents.SerializeToString(&bytestring);
				len = bytestring.length();
				send_message(msg(PSET_MON, len, bytestring.c_str()), me, true);

				bytestring.clear();
				setmonsterContents.clear_data();
			}
		}
		setmonsterContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_MON, len, bytestring.c_str()), me, true);

		bytestring.clear();
		setmonsterContents.clear_data();
	}


	charId_in_room_except_me.clear();
	vec_mon.clear();

	printLog("id : %d, x_off : %d, y_off : %d\n", cur_id, x_off, y_off);

}