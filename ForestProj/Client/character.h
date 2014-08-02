#ifndef CHARACTER_H
#define CHARACTER_H

//client ĳ���� Ŭ����
class Character
{
private:
	int ID;
	int x, y;
	int lv;
	int prtHp, maxHp;
	int power;
	int exp;
	SRWLOCK srw;
public:
	Character()
	{
		ID = -1;
	}
	Character(int x, int y)
	{
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
	PSRWLOCK getLock()
	{
		return &srw;
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
	void setLv(const int lv,const int maxHp,const int power)
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