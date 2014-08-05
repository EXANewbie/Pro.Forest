#ifndef CHARACTER_H
#define CHARACTER_H

//client 캐릭터 클래스
class Character
{
private:
	int ID;
	int x, y;
	std::string name;
	int lv;
	int prtHp, maxHp;
	int power;
	int prtExp, maxExp;
	SRWLOCK srw;
public:
	Character()
	{
		InitializeSRWLock(&srw);
		ID = -1;
		prtExp = 0;
	}
	Character(int x, int y) : Character()
	{
		this->x = x;
		this->y = y;
	}
	
	const int getID()
	{
		return ID;
	}
	std::string getName()
	{
		return name;
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
	const int getPrtExp()
	{
		return prtExp;
	}
	const int getMaxExp()
	{
		return maxExp;
	}
	PSRWLOCK getLock()
	{
		return &srw;
	}

	void setID(const int ID)
	{
		this->ID = ID;
	}
	void setName(std::string name)
	{
		this->name = name;
	}
	void setX(const int x)
	{
		this->x = x;
	}
	void setY(const int y)
	{
		this->y = y;
	}
	void setLv(const int lv, const int maxHp, const int power, const int maxExp)
	{
		this->lv = lv;
		this->maxHp = maxHp;
		this->prtHp = this->maxHp;
		this->power = power;
		this->maxExp = maxExp;
	}
	void setExpUp(const int exp)
	{
		prtExp += exp;
	}
	void setPrtHp(const int hp)
	{
		this->prtHp = hp;
	}
	void setPrtExp(const int exp)
	{
		this->prtExp = exp;
	}

	void attacked(int damage)
	{
		prtHp -= damage;
		if (prtHp <= 0) prtHp = 0;
	}

};



#endif