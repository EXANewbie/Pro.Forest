#include <WinSock2.h>

#include "cmap.h"
#include "mmap.h"
#include "Scoped_Lock.h"

#include "../protobuf/eraseuser.pb.h"

void Handler_PERASE_USER(Character *myChar, std::string* str)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	ERASE_USER::CONTENTS contents;
	contents.ParseFromString(*str);

	for (int i = 0; i < contents.data_size(); ++i)
	{
		auto user = contents.data(i);
		std::string otherName;
		Character* other;
		int id = user.id();
		{
			Scoped_Wlock SW(&chars->srw);
			other = chars->find(id);
			otherName = other->getName();
			chars->erase(id);
		}

		printf("※ 유저 %s님과 멀어져간다!\n", otherName.c_str());

	}
	contents.clear_data();

}
