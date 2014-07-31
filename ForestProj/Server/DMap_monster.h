#ifndef DMAP_MONSTER_H
#define DMAP_MONSTER_H

#include <windows.h>
#include <vector>
#include <list>
#include <map>

#include "monster.h"
#include "Constant.h"

using std::vector;
using std::list;
using std::pair;
using std::map;

class E_List_Mon
{
private:
	list<Monster*> clist;
public:
	SRWLOCK slock;

	E_List_Mon()
	{
		InitializeSRWLock(&slock);
	}

	void push_back(Monster* c)
	{
		clist.push_back(c);
	}

	void erase(Monster* c)
	{
		for (auto i = begin(); i != end(); i++)
		{
			if (*i == c)
			{
				clist.erase(i);
				break;
			}
		}
	}

	void erase(int id)
	{
		for (auto i = begin(); i != end(); i++)
		{
			if ((*i)->getID() == id)
			{
				clist.erase(i);
				break;
			}
		}
	}

	Monster* find(int id)
	{
		Monster* value = nullptr;
		for (auto i = begin(); i != end(); i++)
		{
			if ((*i)->getID() == id)
			{
				value = *i;
				break;
			}
		}
		return value;
	}
	list<Monster*>::iterator begin()	{ return clist.begin(); }
	list<Monster*>::iterator end() { return clist.end(); }
};

class F_Vector_Mon
{
	typedef E_List_Mon* listptr;
private:
	vector<vector<listptr>> vec;

	static F_Vector_Mon* instance;

	F_Vector_Mon(int w, int h)
	{
		for (int i = 0; i <= w; i++)
		{
			vec.push_back(vector<listptr>());
			for (int j = 0; j <= h; j++)
			{
				vec[i].push_back(new E_List_Mon());
			}
		}
	}

	F_Vector_Mon() : F_Vector_Mon(WIDTH, HEIGHT) {}
public:
	static F_Vector_Mon* getInstance()
	{
		if (instance == NULL)
			instance = new F_Vector_Mon;

		return instance;
	}

	listptr get(int x, int y) {
		return vec[x][y];
	}
};

class Directory_Map_Mon
{
	//아직은 쓸일이 없다
};

class Access_Map_Mon
{
private:
	map<int, Monster*> charMap;

	static Access_Map_Mon* instance;
public:
	SRWLOCK slock;
private:
	Access_Map_Mon()
	{
		InitializeSRWLock(&slock);
	}
public:
	static Access_Map_Mon* getInstance()
	{
		if (instance == NULL)
		{
			instance = new Access_Map_Mon();
		}
		return instance;
	}
	
	void insert(int id, Monster* m)
	{
		charMap.insert(pair<int, Monster*>(id, m));
	}
	void erase(int id)
	{
		charMap.erase(id);
	}
	Monster* find(int id)
	{
		auto itr = charMap.find(id);
		if (itr == charMap.end())
		{
			return nullptr;
		}
		else
		{
			return itr->second;
		}
	}
};

#endif