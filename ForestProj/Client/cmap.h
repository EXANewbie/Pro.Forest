#ifndef CMAP_H
#define CMAP_H

#include <map>

#include "character.h"

class SYNCHED_CHARACTER_MAP
{
private:
	typedef std::map<int, Character*> MAP;
	MAP id_char;
	static SYNCHED_CHARACTER_MAP* instance;
	SYNCHED_CHARACTER_MAP()
	{
		InitializeSRWLock(&srw);
	}
public:
	SRWLOCK srw;
	static SYNCHED_CHARACTER_MAP* getInstance()
	{
		if (instance == NULL)
		{
			instance = new SYNCHED_CHARACTER_MAP;
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
	void insert(int key, Character* Char)
	{
		id_char.insert(std::pair<int, Character*>(key, Char));
	}
	void erase(int key)
	{
		delete id_char[key];
		id_char.erase(key);
	}
	Character* find(int key)
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
