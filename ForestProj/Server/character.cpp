#include "character.h"

//class Character
//{
//private:
//	int ID;
//	int x, y;
//public:
//	const int getID();
//	const int getX();
//	const int getY();
//
//	void setID(const int);
//	void setX(const int);
//	void setY(const int);
//};
Character::Character()
{
}

Character::Character(int ID, int x , int y )
{
	this->ID = ID;
	this->x = x;
	this->y = y;
}

const int Character::getID()
{
	return ID;
}

const int Character::getX()
{
	return x;
}

const int Character::getY()
{
	return y;
}

void Character::setID(const int ID)
{
	this->ID = ID;
}

void Character::setX(const int x)
{
	this->x = x;
}

void Character::setY(const int y)
{
	this->y = y;
}

