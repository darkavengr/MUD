#include "size.h"
#include "user.h"

#define SPELL_NAME_COLUMN	0
#define SPELL_POINTS_COLUMN	1
#define SPELL_DAMAGE_COLUMN	2
#define SPELL_MESSAGE_COLUMN	3
#define SPELL_LEVEL_COLUMN	4
#define SPELL_CLASS_COLUMN	5

#ifndef SPELL_H
	#define SPELL_H
	typedef struct {
		char *name[BUF_SIZE];
		int spellpoints;
		int damage;
		char *message[BUF_SIZE];
		int level;
		char *class[BUF_SIZE];
	} spell;
#endif

int CastSpell(user *currentuser,char *spellname,char *target);
int LoadSpells(void);


