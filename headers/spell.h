#include "size.h"

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

