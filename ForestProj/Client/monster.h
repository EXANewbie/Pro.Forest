#ifndef MONSTER_H
#define MONSTER_H

class Knight
{
private:
	int ID;
	int x, y;
	int prtHp, maxHp;
	int power;
public:
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
	const int getMaxHp()
	{
		return maxHp;
	}
	const int getPrtHp()
	{
		return prtHp;
	}

	void setID(const int ID)
	{
		this->ID = ID;
	}
	void setXY(const int x, const int y)
	{
		this->x = x;
		this->y = y;
	}
	void attacked(int damage)
	{
		prtHp -= damage;
		if (prtHp <= 0) prtHp = 0;
	}

};

#endif