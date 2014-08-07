#include <WinSock2.h>

#include "character.h"
#include "Scoped_Lock.h"
#include "cmap.h"
#include "mmap.h"

#include "../protobuf/init.pb.h"

void Handler_PINIT(int* myID, Character *myChar, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	INIT::CONTENTS contents;
	contents.ParseFromString(*str);

	Scoped_Wlock SW(&chars->srw);

	auto user = contents.data(0);
	int id = user.id(), x = user.x(), y = user.y();
	std::string name = user.name();
	int lv = user.lv(), maxHp = user.maxhp(), power = user.power(), maxExp = user.maxexp();
	int prtExp = user.prtexp();
	*myID = id;
	
	Character* myCharacter = myChar;
	myCharacter->setID(id);
	myCharacter->setName(name);
	myCharacter->setX(x);
	myCharacter->setY(y);
	myCharacter->setLv(lv, maxHp, power, maxExp);
	myCharacter->setPrtExp(prtExp);

	chars->insert(id, myChar);
	
	printf("- %s님께서 위치 (%d,%d) 에 생성되었습니다!\n", myChar->getName().c_str(), myChar->getX(), myChar->getY());
	
	contents.clear_data();
}