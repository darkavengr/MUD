#include "room.h"

#define USERNAME      0                                                   /* array entry for user info */
#define PASSWORD      1
#define HOMEROOM      2
#define USERLEVEL     3
#define DESCRIPTION   4
#define MAGICPOINTS   5
#define STAMINAPOINTS  6
#define EXPERIENCEPOINTS  7
#define GENDER 	     8
#define RACE 	     9
#define CLASS        10
#define USERFLAGS    11

#define NOVICE      1                                                     /* user levels */
#define WARRIOR     2
#define HERO        3
#define CHAMPION    4
#define SUPERHERO   5
#define ENCHANTER   6
#define SORCEROR    7
#define NECROMANCER 8
#define LEGEND      9
#define WIZARD      10
#define ARCHWIZARD  11
#define DUNGEONMASTER  12

#define USER_INVISIBLE 1
#define USER_GAGGED  2

#define DEFAULT_STAMINAPOINTS  1000                               /* default number of stamina points */
#define DEFAULT_MAGICPOINTS  1000                               /* default number of magic points */

#define MALE 			0                                                       /* genders */
#define FEMALE 			1

#ifndef USER_H
#define USER_H
typedef struct {
 char *name[BUF_SIZE];
 char *password[BUF_SIZE];
 char *desc[BUF_SIZE];
 int status;
 int gender;
 unsigned int room;
 unsigned int homeroom;
 int magicpoints;
 int staminapoints;
 int experiencepoints;
 int loggedin;
 struct race *race;
 struct class *userclass;
 int handle;
 int flags;
 char *ip[BUF_SIZE];
 struct roomobject *carryobjects;
 struct user *last;
 struct room *roomptr;
 struct user *next;
 char *roomname[BUF_SIZE];
 char *ipaddress[BUF_SIZE];
} user;
#endif

