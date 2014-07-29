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
};

#endif