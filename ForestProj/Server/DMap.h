#ifndef DMAP_H
#define DMAP_H

#include <windows.h>
#include <vector>
#include <list>

#include "character.h"
#include "Constant.h"

using std::vector;
using std::list;

class E_List
{
private :
	typedef Character* Charptr;
	list<Charptr> clist;
public :
	SRWLOCK slock;
private :
	list<Charptr>::iterator begin()	{ return clist.begin(); }
	list<Charptr>::iterator end() { return clist.end(); }
//	void AcquireWrite() { AcquireSRWLockExclusive(&slock); }
//	void AcquireRead() { AcquireSRWLockShared(&slock); }
//	void ReleaseWrite() { ReleaseSRWLockExclusive(&slock); }
//	void ReleaseRead() { ReleaseSRWLockShared(&slock); }
public :
	E_List()
	{
		InitializeSRWLock(&slock);
	}

	void push_back(Charptr c)
	{ 
//		AcquireWrite();
		clist.push_back(c);
//		ReleaseWrite();
	}

	void erase(Charptr c)
	{
//		AcquireWrite();
		for (auto i = begin(); i != end(); i++)
		{
			if (*i == c)
			{
				clist.erase(i);
				break;
			}
		}
//		ReleaseWrite();
	}

	void erase(int id)
	{
//		AcquireWrite();
		for (auto i = begin(); i != end(); i++)
		{
			if ((*i)->getID() == id)
			{
				clist.erase(i);
				break;
			}
		}
//		ReleaseWrite();
	}

	Charptr find(int id)
	{
		Charptr value = nullptr;
//		AcquireRead();
		for (auto i = begin(); i != end(); i++)
		{
			if ((*i)->getID() == id)
			{
				value = *i;
				break;
			}
		}
//		ReleaseRead();

		return value;
	}
};

class F_Vector
{
	typedef E_List Charlist;
	typedef Charlist* listptr;
private :
	vector<listptr>* vec;
	
	static F_Vector* instance;

	F_Vector(int w, int h)
	{
		vec = new vector<listptr>[w];
		for (int i = 0; i <= w; i++)
		{
			for (int j = 0; j <= h; j++)
			{
				vec[i].push_back(new Charlist());
			}
		}
	}

	F_Vector() : F_Vector(WIDTH, HEIGHT) {}
public :
	static F_Vector* getInstance()
	{
		return instance;
	}

	static void makeThis() {
		instance = new F_Vector();
	}

	listptr get(int x, int y) {
		return vec[x][y];
	}
};

class Directory_Map
{

};

#endif