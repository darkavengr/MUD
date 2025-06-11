#include "size.h"

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
 struct race *next;
} race;
#endif

