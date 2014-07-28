#ifndef CHARACTER_H
#define CHARACTER_H

#include "Constant.h"
int bigRand();

class Character
{
private :
	int ID;
	int x, y;
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
};

#endif