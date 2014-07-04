#ifndef CHARACTER_H
#define CHARACTER_H

class Character
{
private :
	int ID;
	int x, y;
public :
	Character();
	Character(int, int=0, int=0);
	const int getID();
	const int getX();
	const int getY();
	
	void setID(const int);
	void setX(const int);
	void setY(const int);

	~Character() 
	{
	}
};

#endif