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

	auto user = contents.data(0);
	int id = user.id(), x = user.x(), y = user.y();
	int lv = user.lv(), maxHp = user.maxhp(), power = user.power(), maxexp = user.maxexp();
	*myID = id;
	Character* myCharacter = new Character();
	myCharacter->setID(id);
	myCharacter->setX(x);
	myCharacter->setY(y);
	myCharacter->setLv(lv, maxHp, power, maxexp);
	*myChar = *myCharacter;
	{
		Scoped_Wlock SW(&chars->srw);
		chars->insert(id, myChar);
	}
	printf("My char id : %d, (%d,%d)\n", id, myChar->getX(), myChar->getY());
	//printf("my lv : %d, hp : %d, power : %d, exp : %d\n",myCharacter->getLv(),myCharacter->getPrtHp(),myCharacter->getPower(),myCharacter->getExp());
	contents.clear_data();

}