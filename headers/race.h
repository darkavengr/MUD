#include "size.h"

#define RACE_NAME_COLUMN	0
#define RACE_MAGIC_COLUMN	1
#define RACE_STRENGTH_COLUMN	2
#define RACE_AGILITY_COLUMN	3
#define RACE_DEXTERITY_COLUMN	4
#define RACE_LUCK_COLUMN	5
#define RACE_WISDOM_COLUMN	6
#define RACE_INTELLIGENCE_COLUMN	7
#define RACE_STAMINA_COLUMN	8

#ifndef RACE_H
	#define RACE_H

	typedef struct race {				/* race */
		char *name[BUF_SIZE];
		int magic;
		int strength;
		int agility;
		int dexterity;
		int luck;
		int wisdom;
		int intelligence;
		int stamina;
	} race;
#endif

