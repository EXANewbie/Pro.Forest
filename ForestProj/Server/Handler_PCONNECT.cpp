#include <string>

#include "../protobuf/connect.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/setmonster.pb.h"

#include "Check_Map.h"
#include "Completion_Port.h"
#include "character.h"
#include "Sock_set.h"
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

using std::string;

void send_message(msg, vector<Character *> &, bool);
void make_vector_id_in_room_except_me(Character*, vector<Character *>&, bool);
void make_monster_vector_in_room(Character* myChar, vector<Monster *>& send_list, bool autolocked);

void Handler_PCONNECT(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, string* readContents)
{
	Sock_set *sock_set = Sock_set::getInstance();
	auto FVEC = F_Vector::getInstance();
	auto AMAP = Access_Map::getInstance();
	auto CMap = Check_Map::getInstance();
	F_Vector_Mon *FVEC_M = F_Vector_Mon::getInstance();
	Access_Map_Mon * AMAP_M = Access_Map_Mon::getInstance();

	CONNECT::CONTENTS connect;
	INIT::CONTENTS initContents;
	SET_USER::CONTENTS setuserContents;
	SET_MONSTER::CONTENTS setmonsterContents;

	string bytestring;
	int len;
	vector<Character *> receiver;
	vector<Character *> me;
	vector<Monster *> vec_mon;

	connect.ParseFromString(*readContents);
	if (connect.data() != "HELLO SERVER!")
	{
		//��¥ Ŭ���̾�Ʈ
	}
	int char_id;
	int x, y, lv, maxHp, power, exp;
	static int id = 0;

	// ĳ���� ��ü�� ���� ��
	// ĳ���� �����ϰ� init �ϴ� �Ϳ� ���ؼ� char lock�� �ʿ䰡 ����.
	char_id = InterlockedIncrement((unsigned *)&id);
	Character* myChar = new Character(char_id);
	myChar->setLv(1, HpPw[0][0], HpPw[0][1]);
	myChar->setExp(0);
	myChar->setSock(handleInfo->hClntSock);
	me.push_back(myChar);

	x = myChar->getX();
	y = myChar->getY();
	lv = myChar->getLv();
	maxHp = myChar->getMaxHp();
	power = myChar->getPower();
	exp = myChar->getExp();

	ioInfo->id = char_id;
	ioInfo->myCharacter = myChar;

	E_List* elist = FVEC->get(x, y);

	{
		Scoped_Wlock SW1(&AMAP->slock);
		Scoped_Wlock SW2(&elist->slock);
		AMAP->insert(char_id, myChar);
		CMap->insert(handleInfo->hClntSock, char_id);
		sock_set->erase(handleInfo->hClntSock);
		elist->push_back(myChar);
	}

	// x�� y�� �ʱⰪ�� �����´�.   
	{
		auto myData = initContents.mutable_data()->Add();
		myData->set_id(char_id);
		myData->set_x(x);
		myData->set_y(y);
		myData->set_lv(lv);
		myData->set_maxhp(maxHp);
		myData->set_power(power);
		myData->set_exp(exp);
	}

	initContents.SerializeToString(&bytestring);
	len = bytestring.length();

	{
		Scoped_Rlock SR(&elist->slock);
		send_message(msg(PINIT, len, bytestring.c_str()), me, true);
	}
	receiver.clear();
	bytestring.clear();
	initContents.clear_data();

	// ���� ������ ĳ������ ������ �ٸ� ������ �����鿡�� �����Ѵ�.
	{
		auto myData = setuserContents.mutable_data()->Add();
		myData->set_id(char_id);
		myData->set_x(x);
		myData->set_y(y);
		myData->set_lv(lv);
		myData->set_maxhp(maxHp);
		myData->set_power(power);
		myData->set_exp(exp);
	}

	setuserContents.SerializeToString(&bytestring);
	len = bytestring.length();

	{
		Scoped_Rlock SR(&elist->slock);

		// ���� �ִ� �濡 �ִ� ģ���鿡�� ���� �������� �˸���.
		// ���� �����濡 �ִ� ģ������ ����?
		make_vector_id_in_room_except_me(myChar, receiver, false/*autolock*/);

		send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, false);
		// ���� ������ char�� ���ؼ� �ڷᱸ���� �־��־��� ������������ �ٸ� �������� �˷ȴ�. �������� char �����ؼ� lock�� ����� �ڴ�.
		// �ٵ� ���� ���� ����.. ĳ���͸� read write �ϴ� ���� �ؾ��ϴµ� �׷� ���� ������. �� �Ǵ� �³���?
		// RE : �±�!!

		setuserContents.clear_data();
		bytestring.clear();

		// PCONNECT�� ������ �������� ���� �濡�ִ� �������� ������ �����Ѵ�.
		for (int i = 0; i < receiver.size(); i++) {
			auto tmpChar = receiver[i];

			auto tempData = setuserContents.mutable_data()->Add();
			tempData->set_id(tmpChar->getID());
			tempData->set_x(tmpChar->getX());
			tempData->set_y(tmpChar->getY());
			tempData->set_lv(lv);
			tempData->set_maxhp(maxHp);
			tempData->set_power(power);
			tempData->set_exp(exp);

			if (setuserContents.data_size() == SET_USER_MAXIMUM) // SET_USER_MAXIMUM�� �Ѱ�ġ�� �����Ϸ��� �� ��
			{
				setuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				send_message(msg(PSET_USER, len, bytestring.c_str()), me, false);

				setuserContents.clear_data();
				bytestring.clear();
			}
		}
		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_USER, len, bytestring.c_str()), me, true);

		receiver.clear();
		setuserContents.clear_data();
		bytestring.clear();
	}

	//�����濡 �ִ� ������ ������ �����Ѵ�.
	E_List_Mon* elist_m = FVEC_M->get(x, y);
	{
		Scoped_Rlock SR(&elist_m->slock);
		make_monster_vector_in_room(myChar, vec_mon, false);
		for (int i = 0; i < vec_mon.size(); ++i)
		{
			Monster* tmpMon = vec_mon[i];
			{
				Scoped_Wlock(tmpMon->getLock());
				tmpMon->SET_BATTLE_MODE();
			}
			auto setmon = setmonsterContents.add_data();
			setmon->set_id(tmpMon->getID());
			setmon->set_x(tmpMon->getX());
			setmon->set_y(tmpMon->getY());
			setmon->set_name(tmpMon->getName());
			setmon->set_lv(tmpMon->getLv());
			setmon->set_maxhp(tmpMon->getMaxHp());
			setmon->set_power(tmpMon->getPower());
			setmon->set_exp(tmpMon->getExp());

			if (setmonsterContents.data_size() == SET_MONSTER_MAXIMUM)
			{
				setmonsterContents.SerializeToString(&bytestring);
				len = bytestring.length();

				send_message(msg(PSET_MON, len, bytestring.c_str()), me, false);
				setmonsterContents.clear_data();
				bytestring.clear();
			}
		}
		setmonsterContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_MON, len, bytestring.c_str()), me, false);

		vec_mon.clear();
		setmonsterContents.clear_data();
		bytestring.clear();
	}
}