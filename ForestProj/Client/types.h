#ifndef TYPES_H
#define TYPES_H

enum TYPE {
	PCONNECT, PINIT, PSET_USER, PMOVE_USER, PDISCONN, PERASE_USER, PSET_MON, PERASE_MON, PUSER_ATTCK, PUSER_ATTCK_RESULT, PUSER_SET_LV, PUSER_SET_EXP, PMONSTER_ATTACK_RESULT
};

enum CAST {
	NOT_CAST, SINGLE_CAST, MULTI_CAST, BROAD_CAST
};

#endif