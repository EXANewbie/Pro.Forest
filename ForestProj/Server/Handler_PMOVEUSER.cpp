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
	int next_x = 0;
	int next_y = 0;
	bool ordered = false;

	auto user = moveuserContents.data(0);
	cur_id = user.id();
	x_off = user.xoff();
	y_off = user.yoff();

	if (x_off + y_off < 0)
	{
		ordered = false;
	}
	else
	{
		ordered = true;
	}

	moveuserContents.clear_data();

	E_List *now_elist, *next_elist;
	E_List_Mon *now_elist_mon, *next_elist_mon;

	{
		Scoped_Rlock CHARACTER_READ_LOCK(pCharacter->getLock());
		int x = pCharacter->getX(), y = pCharacter->getY();

		if (pCharacter->getPrtHp() == 0)
		{
			//죽은 상태에서 move 명령은 유효하지 않다. 따라서 return;
			return;
		}
		
		if (Boundary_Check(cur_id, x, y, x_off, y_off) == false)
		{
			return;
		}

		next_x = x + x_off;
		next_y = y + y_off;

		now_elist = FVEC->get(x, y);
		now_elist_mon = FVEC_M->get(x, y);

		next_elist = FVEC->get(next_x, next_y);
		next_elist_mon = FVEC_M->get(next_x, next_y);
	}

	/* 경계값 체크 로직 */

	// 지금 내가 있는 방에 몬스터가 있는지 확인을 하도록 하자. 몬스터와 함께있으면 못움직이게 할 것이기 때문.
	{
		E_List *first, *second;
		E_List_Mon *first_m, *second_m;

		if (ordered == true)
		{
			first = now_elist;
			first_m = now_elist_mon;

			second = next_elist;
			second_m = next_elist_mon;
		}
		else
		{
			first = next_elist;
			first_m = next_elist_mon;

			second = now_elist;
			second_m = now_elist_mon;
		}

		Scoped_Wlock NOW_E_LIST_WRITE_LOCK(&first->slock);
		Scoped_Wlock NOW_E_LIST_MON_WRITE_LOCK(&first_m->slock);

		Scoped_Wlock NEXT_E_LIST_WRITE_LOCK(&second->slock);
		Scoped_Wlock NEXT_E_LIST_MON_WRITE_LOCK(&second_m->slock);

		if (pCharacter->getPrtHp() == 0)
		{
			//죽은 상태에서 move 명령은 유효하지 않다. 따라서 return;
			return;
		}

		if (!now_elist_mon->empty())
		{
			//몬스터가 있는데!! 왜 움직이려고!! 응!!?
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

			if (eraseuserContents.data_size() == ERASE_USER_MAXIMUM) // ERASE_USER_MAXIMUM이 한계치로 접근하려고 할 때
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

		// 기존 방의 유저들의 정보를 지운다.
		send_message(msg(PERASE_USER, len, bytestring.c_str()), me, true);

		bytestring.clear();
		eraseuserContents.clear_data();

		// 기존 방의 유저들에게 내가 사라짐을 알림
		auto eraseuser = eraseuserContents.add_data();
		eraseuser->set_id(cur_id);
		eraseuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PERASE_USER, len, bytestring.c_str()), charId_in_room_except_me, true);

		eraseuserContents.clear_data();

		// 기존 방의 몬스터들의 정보를 지운다.
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

		charId_in_room_except_me.clear();
		vec_mon.clear();

		now_elist->erase(cur_id);

		{
			// 캐릭터를 해당 좌표만큼 이동시킴
			Scoped_Wlock CHARACTER_WRITE_LOCK(pCharacter->getLock());
			pCharacter->setX(next_x);
			pCharacter->setY(next_y);
		}

		next_elist->push_back(pCharacter);

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

		make_vector_id_in_room_except_me(pCharacter, charId_in_room_except_me, false/*autolock*/);

		// 새로운 방의 유저들에게 내가 등장함을 알림

		int x;
		int y;
		int lv;
		int prtHp;
		int maxHp;
		int power;
		int prtExp;
		int maxExp;
		{
			Scoped_Rlock CHARACTER_READ_LOCK(pCharacter->getLock());
			x = pCharacter->getX(), y = pCharacter->getY();
			lv = pCharacter->getLv();
			prtHp = pCharacter->getPrtHp();
			maxHp = pCharacter->getMaxHp();
			power = pCharacter->getPower();
			prtExp = pCharacter->getPrtExp();
			maxExp = pCharacter->getMaxExp();
		}
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

			{
				Scoped_Rlock OTHER_CHARACTER_READ_LOCK(tmpChar->getLock());
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
			}

			if (setuserContents.data_size() == SET_USER_MAXIMUM)
			{
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

		make_monster_vector_in_room(pCharacter, vec_mon, false);

		for (int i = 0; i < vec_mon.size(); ++i)
		{
			Monster* tmpMon = vec_mon[i];
			{
				Scoped_Wlock MONSTER_WRITE_LOCK(tmpMon->getLock());
				tmpMon->SET_BATTLE_MODE();
			}

			auto setmon = setmonsterContents.add_data();
			{
				Scoped_Wlock MONSTER_READ_LOCK(tmpMon->getLock());
				setmon->set_id(tmpMon->getID());
				setmon->set_x(tmpMon->getX());
				setmon->set_y(tmpMon->getY());
				setmon->set_name(tmpMon->getName());
				setmon->set_lv(tmpMon->getLv());
				setmon->set_prthp(tmpMon->getPrtHp());
				setmon->set_maxhp(tmpMon->getMaxHp());
				setmon->set_power(tmpMon->getPower());
			}

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