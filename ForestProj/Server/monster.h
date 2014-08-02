#ifndef MONSTER_H
#define MONSTER_H

#include <WinSock2.h>
#include <string>
#include "Constant.h"
#include "types.h"

int bigRand();

class Monster
{
public:	
	Monster() = default;
	virtual const int getID() = 0;
	virtual const int getName() = 0;
	virtual const int getX() = 0;
	virtual const int getY() = 0;
	virtual const int getLv() = 0;
	virtual const int getMaxHp() = 0;
	virtual const int getPrtHp() = 0;
	virtual const int getPower() = 0;
	virtual const int getExp() = 0;
	virtual const int getState() = 0;

	virtual void setID(const int ID) = 0;
	virtual void setX(const int x) = 0;
	virtual void setY(const int y) = 0;
	virtual void setLv(const int lv, const int maxHp, const int power) = 0;
	virtual void setExp(const int exp) = 0;
	virtual void setState(const int state) = 0;
	virtual void attacked(int damage) = 0;

	virtual void getNextOffset(int bef_x_off, int bef_y_off, int *nxt_x_off, int *nxt_y_off) = 0;
	virtual void SET_BATTLE_MODE() = 0;
	virtual void SET_PEACE_MODE() = 0;

	virtual void CONTINUE_BATTLE_MODE(int user_id,int attack_type) = 0;
	virtual void CONTINUE_PEACE_MODE(int bef_x_off,int bef_y_off) = 0;

	virtual PSRWLOCK getLock() = 0;
};

class Knight : public Monster
{
private:
	int name ;
	int ID;
	int x, y;
	int lv;
	int prtHp, maxHp;
	int power;
	int exp;
	int state;

	SRWLOCK srw;
public:
	Knight() : Monster() { name = 1; }
	Knight(int x, int y) : Monster()
	{
		name = 1;
		this->x = x;
		this->y = y;
		state = NULL;
	}
	Knight(int ID) : Knight(bigRand() % (WIDTH + 1), bigRand() % (HEIGHT + 1))
	{
		this->ID = ID;
		state = NULL;
		InitializeSRWLock(&srw);
	}
	const int getName()
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
	const int getState()
	{
		return state;
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
	void setState(const int state)
	{
		this->state = state;
	}

	void attacked(int damage)
	{
		prtHp -= damage;
		if (prtHp <= 0) prtHp = 0;
	}

	PSRWLOCK getLock()
	{
		return &srw;
	}

	void getNextOffset(int bef_x_off, int bef_y_off, int *nxt_x_off, int *nxt_y_off);

	void SET_BATTLE_MODE();
	void SET_PEACE_MODE();

	void CONTINUE_BATTLE_MODE(int user_id, int attack_type);
	void CONTINUE_PEACE_MODE(int bef_x_off, int bef_y_off);
};

#endif