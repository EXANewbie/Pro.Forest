#ifndef CHARACTER_H
#define CHARACTER_H

class Character
{
private :
	int ID;
	int x, y;
public :
	Character()
	{
	}
	Character(int ID=0, int x=0, int y=0)
	{
		this->ID = ID;
		this->x = x;
		this->y = y;
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