#ifndef SCOPED_LOCK_H
#define SCOPED_LOCK_H

#include <Windows.h>

class Scoped_Rlock {
private :
	PSRWLOCK c;
public :
	Scoped_Rlock(PSRWLOCK s) {
		c = s;
		AcquireSRWLockShared(c);
	}
	~Scoped_Rlock() {
		ReleaseSRWLockShared(c);
	}
};

class Scoped_Wlock {
private :
	PSRWLOCK c;
public :
	Scoped_Wlock(PSRWLOCK s) {
		c = s;
		AcquireSRWLockExclusive(c);
	}
	~Scoped_Wlock() {
		ReleaseSRWLockExclusive(c);
	}
};

#endif