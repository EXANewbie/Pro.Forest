#ifndef CHARACTER_H
#define CHARACTER_H

#include <WinSock2.h>
#include "Constant.h"
int bigRand();

class Character
{
private :
	SOCKET sock;
	int ID;
	int x, y;
	int lv;
	int prtHp, maxHp;
	int power;
	int exp;
public :
	Character() = default;
	Character(int x, int y)
	{
		this->x = x;
		this->y = y;
	}
	Character(int ID) : Character(bigRand()%(WIDTH+1), bigRand()%(HEIGHT+1))
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
	const int getExp()
	{
		return exp;
	}

	void setSock(const SOCKET sock)
	{
		this->sock = sock;
	}
	void setID(const int ID)
	{
		this->ID = ID;
	}
	void setX(const int x)
	{
		this->x = x;
	}
	void setY(const int y)
	{
		this->y = y;
	}
	void setLv(const int lv, const int maxHp, const int power)
	{
		this->lv = lv;
		this->maxHp = maxHp;
		this->prtHp = this->maxHp;
		this->power = power;
	}
	void setExp(const int exp)
	{
		this->exp = exp;
	}

	void attacked(int damage)
	{
		prtHp -= damage;
		if (prtHp <= 0) prtHp = 0;
	}
};

#endif