#include "size.h"

#define MONSTER_SPAWN_PROB	5
#define ROOM_MONSTER_COUNT	10

#ifndef MONSTER_H
	#define MONSTER_H

	typedef struct {			/* monsters */
		char *name[BUF_SIZE];
		char *desc[BUF_SIZE];
		struct room *roomptr;
		int stamina;
		int evil;
		char *arrivemessage[BUF_SIZE];
		char *leavemessage[BUF_SIZE];
		char *createmessage[BUF_SIZE];
		char *diemessage[BUF_SIZE];
		int moveodds;
		int genodds;
		int damage;
		int experiencepoints;
		int room;
		int sleep;
		struct monster *last;
		struct monster *next;
	} monster;
#endif

