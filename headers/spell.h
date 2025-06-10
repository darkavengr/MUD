#include "size.h"
#include "user.h"

int castspell(user *currentuser,char *s,char *t);
int loadspells(void);

#ifndef SPELL_H
#define SPELL_H
typedef struct {
 char *name[BUF_SIZE];
 int spellpoints;
 int damage;
 char *message[BUF_SIZE];
 int level;
 char *class[BUF_SIZE];
 struct spell *next;
} spell;
#endif

