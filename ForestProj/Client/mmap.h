#ifndef MMAP_H
#define MMAP_H

#include <map>

#include "monster.h"

class SYNCHED_MONSTER_MAP
{
private:
	typedef std::map<int, Monster*> MAP;
	MAP id_char;
	static SYNCHED_MONSTER_MAP* instance;
	SYNCHED_MONSTER_MAP()
	{
		InitializeSRWLock(&srw);
	}
public:
	SRWLOCK srw;
	static SYNCHED_MONSTER_MAP* getInstance()
	{
		if (instance == NULL)
		{
			instance = new SYNCHED_MONSTER_MAP;
		}
		return instance;
	}
	MAP::iterator begin()
	{
		return id_char.begin();
	}
	MAP::iterator end()
	{
		return id_char.end();
	}
	int size()
	{
		return id_char.size();
	}
	void insert(int key, Monster* Char)
	{
		id_char.insert(std::pair<int, Monster*>(key, Char));
	}
	void erase(int key)
	{
		delete id_char[key];
		id_char.erase(key);
	}
	Monster* find(int key)
	{
		auto ret = id_char.find(key);
		if (ret == id_char.end()) return NULL;
		else return ret->second;
	}
	void clear()
	{
		id_char.clear();
	}
};


#endif
