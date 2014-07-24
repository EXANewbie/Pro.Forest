#ifndef CHARACTER_H
#define CHARACTER_H

class Character
{
private :
	int ID;
	int x, y;
public :
	const int getID();
	const int getX();
	const int getY();
	
	void setID(const int);
	void setX(const int);
	void setY(const int);
};

#endif