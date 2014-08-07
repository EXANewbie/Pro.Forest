#ifndef MONSTER_H
#define MONSTER_H

#include <string>

int bigRand();

class Monster
{
public:
	Monster() = default;
	virtual const int getID() = 0;
	virtual std::string getName() = 0;
	virtual const int getX() = 0;
	virtual const int getY() = 0;
	virtual const int getLv() = 0;
	virtual const int getMaxHp() = 0;
	virtual const int getPrtHp() = 0;
	virtual const int getPower() = 0;
	virtual const int getExp() = 0;

	virtual void setID(const int ID) = 0;
	virtual void setX(const int x) = 0;
	virtual void setY(const int y) = 0;
	virtual void setLv(const int lv, const int maxHp, const int power) = 0;
	virtual void setExp(const int exp) = 0;
	virtual void setPrtHp(const int hp) = 0;
	virtual void attacked(int damage) = 0;
	
};

class Knight : public Monster
{
private:
	std::string name;
	int ID;
	int x, y;
	int lv;
	int prtHp, maxHp;
	int power;
	int exp;
public:
	Knight() : Monster() { name = "기사"; }
	Knight(int x, int y) : Monster()
	{
		name = "기사";
		this->x = x;
		this->y = y;
	}
	Knight(int ID) : Monster()
	{
		name = "기사" ;
		this->ID = ID;
	}
	std::string getName()
	{
		return name;
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
	void setPrtHp(const int hp)
	{
		this->prtHp = hp;
	}

	void attacked(int damage)
	{
		prtHp -= damage;
		if (prtHp <= 0) prtHp = 0;
	}

};

#endif