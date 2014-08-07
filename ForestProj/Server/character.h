#ifndef CHARACTER_H
#define CHARACTER_H

#include <WinSock2.h>
#include "Constant.h"

using std::string;

int bigRand();

class Character 
{
private :
	SOCKET sock;
	int ID;
	std::string name;
	int x, y;
	int lv;
	int prtHp, maxHp;
	int power;
	int prtExp, maxExp;
	SRWLOCK srw;
public :
	Character()
	{
		InitializeSRWLock(&srw);
		prtExp = 0;
	}
	Character(int x, int y) :Character()
	{
		this->x = x;
		this->y = y;
	}
	Character(int ID) : Character(/*bigRand()%(WIDTH+1)*/22, /*bigRand()%(HEIGHT+1)*/22)
	{
		this->ID = ID;
	}
	const int getSock()
	{
		return sock;
	}
	const int getID()
	{
		return ID;
	}
	std::string getName()
	{
		return name;
	}
	const int getX()
	{
		return x;
	}
	const int getY()
	{
		return y;
	}
	const int getLv()
	{
		return lv;
	}
	const int getMaxHp()
	{
		return maxHp;
	}
	const int getPrtHp()
	{
		return prtHp;
	}
	const int getPower()
	{
		return power;
	}
	const int getPrtExp()
	{
		return prtExp;
	}
	const int getMaxExp()
	{
		return maxExp;
	}

	void setSock(const SOCKET sock)
	{
		this->sock = sock;
	}
	void setID(const int ID)
	{
		this->ID = ID;
	}
	void setName(std::string name)
	{
		this->name = name;
	}
	void setX(const int x)
	{
		this->x = x;
	}
	void setY(const int y)
	{
		this->y = y;
	}
	void setPrtHp(const int prtHp)
	{
		this->prtHp = prtHp;
	}
	void setLv(const int lv, const int maxHp, const int power, const int maxExp)
	{
		this->lv = lv;
		this->maxHp = maxHp;
		this->prtHp = this->maxHp;
		this->power = power;
		this->maxExp = maxExp;
	}
	void setExpUp(const int exp)
	{
		prtExp += exp;
	}

	void setHPMax() {
		prtHp = maxHp;
	}

	void attacked(int damage)
	{
		prtHp -= damage;
		if (prtHp <= 0) prtHp = 0;
	}

	PSRWLOCK getLock()
	{
		return &srw;
	}
};

#endif