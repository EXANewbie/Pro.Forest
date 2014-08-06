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
void init_proc(Character* myChar);

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
		//가짜 클라이언트
	}
	int char_id;
	int x, y, lv, maxHp, power, maxexp, prtExp;
	std::string name;
	static int id = 0;

	// 캐릭터 객체를 생성 후
	// 캐릭터 생성하고 init 하는 것에 대해선 char lock할 필요가 없다.
	char_id = InterlockedIncrement((unsigned *)&id);
	Character* myChar = new Character(char_id);
	myChar->setLv(1, HpPw[1][0], HpPw[1][1], maxExp[1]);
	myChar->setSock(handleInfo->hClntSock);
	myChar->setName(connect.name());

	//		Scoped_Wlock SW1(&AMAP->slock);
	AMAP->insert(char_id, myChar);
	CMap->insert(handleInfo->hClntSock, char_id);
	sock_set->erase(handleInfo->hClntSock);
	ioInfo->id = char_id;
	ioInfo->myCharacter = myChar;
	
	init_proc(myChar);
}