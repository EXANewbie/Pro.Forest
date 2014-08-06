//PMODEDEADRESPAWN

#include <string>

#include "../protobuf/userrespawn.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/setmonster.pb.h"
#include "../protobuf/init.pb.h"


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

void make_vector_id_in_room_except_me(Character *, vector<Character *>&, bool);
void make_vector_id_in_room(E_List *, vector<Character *>&);
void send_message(msg, vector<Character *> &, bool);
void unpack(msg, char *, int *);
void set_single_cast(Character *, vector<Character *>&);
void make_monster_vector_in_room(Character* myChar, vector<Monster *>& send_list, bool autolocked);
void init_proc(Character* myChar);

void Handler_USERRESPAWN(LPPER_IO_DATA ioInfo, string* readContents) {
	USER_RESPAWN::CONTENTS respawnmsg;
	SET_USER::CONTENTS setuserContents;
	SET_MONSTER::CONTENTS setmonsterContents;
	INIT::CONTENTS initContents;
	
	string bytestring;
	int len;
	vector<Character *> receiver, me;

	respawnmsg.ParseFromString(*readContents);

	if (ioInfo->block != nullptr)
	{
		Memory_Pool::getInstance()->pushBlock(ioInfo->block);
		ioInfo->block = nullptr;
	}
	ioInfo_Pool::getInstance()->pushBlock(ioInfo);

	auto FVEC = F_Vector::getInstance();

	int ID = respawnmsg.id();
	int x = respawnmsg.x();
	int y = respawnmsg.y();

	auto AMAP = Access_Map::getInstance();

	Character* myChar = nullptr;
	{
		myChar = AMAP->find(ID);

		if (myChar == nullptr) {
			puts("유저 나감 ㅡ.ㅡ");
			// 죽었다고 나간 노매너 유저임. 흥!!
			return;
		}
	}
	
	// 힐!! 체력 만빵!!
	myChar->setHPMax();
	myChar->setX(x);
	myChar->setY(y);

	init_proc(myChar);
}