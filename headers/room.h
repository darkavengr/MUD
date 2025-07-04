#include "monster.h"
#define OBJECT_NAME 		0                                                 /* array entry for object info */
#define OBJECT_STAMINAPOINTS	1
#define OBJECT_MAGICPOINTS 	2
#define OBJECT_ATTACKPOINTS	3
#define OBJECT_GENERATEPROB	4
#define OBJECT_DESCRIPTION	5
#define OBJECT_VERB		6
#define OBJECT_VERB_MESSAGE	7
#define OBJECT_OWNER		8
#define OBJECT_ATTR		9

#define OBJECT_DELETE_OWNER	1                                          /* object properties */
#define OBJECT_DELETE_PUBLIC	2
#define OBJECT_MOVEABLE_OWNER	4
#define OBJECT_MOVEABLE_PUBLIC	8
#define OBJECT_PICKUP_OWNER	16
#define OBJECT_PICKUP_PUBLIC	32
#define OBJECT_RENAME_OWNER	64
#define OBJECT_RENAME_PUBLIC	128
#define OBJECT_TEMPORARY	256

#define ROOM_CREATE_OWNER	1                                            /* room properties */
#define ROOM_CREATE_PUBLIC	2
#define ROOM_EXIT_OWNER		4
#define ROOM_EXIT_PUBLIC	8
#define ROOM_RENAME_OWNER	16
#define ROOM_RENAME_PUBLIC	32
#define ROOM_HAVEN		64
#define ROOM_PRIVATE		128
#define ROOM_DEAD		256

#define NUMBER_OF_DIRECTIONS	9

#ifndef ROOM_H
#define ROOM_H
typedef struct roomobject {			/* objects */
 unsigned int id;
 char *owner[BUF_SIZE];
 char *name[BUF_SIZE];
 char *desc[BUF_SIZE];
 int attackpoints;
 int generateprob;
 int staminapoints;
 int magicpoints;
 int attr;
 char *verb[BUF_SIZE];
 char *verbmessage[BUF_SIZE];
 struct roomobject *next;
} roomobject;

typedef struct room {				/* rooms */
 unsigned int id;
 char *name[BUF_SIZE];
 char *owner[BUF_SIZE];
 char *desc[BUF_SIZE];
 int attr;
 int exits[11];
 roomobject *roomobjects;
 roomobject *roomobjects_last;
 int monstercount;
 monster *roommonsters;
 monster *roommonsters_last;
} room;
#endif

