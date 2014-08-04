#include <string>

#include "../protobuf/peacemove.pb.h"
#include "../protobuf/setmonster.pb.h"

#include "Completion_Port.h"
#include "DMap.h"
#include "Scoped_Lock.h"
#include "Memory_Pool.h"
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

void make_vector_id_in_room(E_List *, vector<Character *>&);
void send_message(msg, vector<Character *> &, bool);
void unpack(msg, char *, int *);

void Handler_PEACEMOVE(LPPER_IO_DATA ioInfo, string* readContents) {
	auto FVEC_M = F_Vector_Mon::getInstance();

	if (ioInfo->block != nullptr)
	{
		Memory_Pool::getInstance()->pushBlock(ioInfo->block);
	}
	ioInfo_Pool::getInstance()->pushBlock(ioInfo);
	PEACEMOVE::CONTENTS peacemove;

	peacemove.ParseFromString(*readContents);
	int ID = peacemove.id();

	auto AMAP_MON = Access_Map_Mon::getInstance();
	Monster* monster;
	{
		Scoped_Rlock SR(&AMAP_MON->slock);
		monster = AMAP_MON->find(ID);

		if (monster == nullptr) {
			//���Ͱ� ��ȿ���� �ʽ��ϴ�.
		}
		//���� ���� ���¿� ��Ŷ�� ���°� ��ġ���� �ʽ��ϴ�!!
		if (monster->getState() != peacemove.state())
		{
			return;
		}

	}

	{
		Scoped_Wlock SW(monster->getLock());
		int bef_x_off, bef_y_off;

		if (peacemove.has_xoff() == true && peacemove.has_yoff() == true)
		{
			bef_x_off = peacemove.xoff();
			bef_y_off = peacemove.yoff();
		}
		else
		{
			bef_x_off = 0;
			bef_y_off = 0;
		}

		int nxt_x_off = 0, nxt_y_off = 0;

		monster->getNextOffset(bef_x_off, bef_y_off, &nxt_x_off, &nxt_y_off);

		// �� �̻� ������ ���� ���� ��� ��.��
		if (nxt_x_off == 0 && nxt_y_off == 0)
		{

		}
		else
		{
			{
				auto elist = FVEC_M->get(monster->getX(), monster->getY());
				Scoped_Wlock SW(&elist->slock);
				elist->erase(monster);
			}
			monster->setX(monster->getX() + nxt_x_off);
			monster->setY(monster->getY() + nxt_y_off);
			{
				auto elist = FVEC_M->get(monster->getX(), monster->getY());
				Scoped_Wlock SW(&elist->slock);
				elist->push_back(monster);
			}

			auto elist = F_Vector::getInstance()->get(monster->getX(), monster->getY());
			Scoped_Rlock SR(&elist->slock);
			int size = elist->size();

			// ������ �����մϴ�!! W.A.R.N.I.N.G !! W.A.R.N.I.N.G !!
			if (size > 0)
			{
				vector<Character *> receiver;
				make_vector_id_in_room(elist, receiver);
				
				string bytestring;
				SET_MONSTER::CONTENTS setmonsterContents;
				auto data = setmonsterContents.add_data();
				data->set_id(monster->getID());
				data->set_name(monster->getName());
				data->set_lv(monster->getLv());
				data->set_maxhp(monster->getMaxHp());
				data->set_power(monster->getPower());
				data->set_x(monster->getX());
				data->set_y(monster->getY());
				
				setmonsterContents.SerializeToString(&bytestring);
				auto message = msg(PSET_MON, bytestring.size(), bytestring.c_str());

				send_message(message, receiver,false);
				// ���⿡ ��� �����鿡�� ���� ���縦 �˷���ߵǿ�~

				monster->SET_BATTLE_MODE();
				// ���⼭���� ��Ʋ��尡 �Ǿ��ٴ°� Ÿ�̸ӿ��� ���������~
			}
			else
			{
				monster->CONTINUE_PEACE_MODE(nxt_x_off, nxt_y_off);
				// �̵��ص� �ǿ�~~ ^-^
			}
		}
	}

}