#include "monster.h"

#define ROOM_OBJECT_ID_COLUMN			0
#define ROOM_OBJECT_ROOM_COLUMN			1
#define ROOM_OBJECT_NAME_COLUMN			2
#define ROOM_OBJECT_DESCRIPTION_COLUMN		3
#define ROOM_OBJECT_OWNER_COLUMN		4
#define ROOM_OBJECT_STAMINA_POINTS_COLUMN	5
#define ROOM_OBJECT_MAGIC_POINTS_COLUMN		6
#define ROOM_OBJECT_ATTRIBUTES_COLUMN		7
#define ROOM_OBJECT_VERB_COLUMN			8
#define ROOM_OBJECT_VERBMESSAGE_COLUMN		9

#define INVOBJECT_NAME 		0                                                 /* array entry for object info */
#define INVOBJECT_ATTRIBUTES	1
#define INVOBJECT_DESCRIPTION	2
#define INVOBJECT_OWNER		3

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

#define ROOM_ID_COLUMN			0
#define ROOM_NAME_COLUMN		1
#define ROOM_OWNER_COLUMN		2
#define ROOM_ATTRIBUTES_COLUMN		3
#define ROOM_DESCRIPTION_COLUMN		4
#define ROOM_NORTH_COLUMN		5
#define ROOM_SOUTH_COLUMN		6
#define ROOM_EAST_COLUMN		7
#define ROOM_WEST_COLUMN		8
#define ROOM_NORTHEAST_COLUMN		9
#define ROOM_NORTHWEST_COLUMN		10
#define ROOM_SOUTHEAST_COLUMN		11
#define ROOM_SOUTHWEST_COLUMN		12
#define ROOM_UP_COLUMN			13
#define ROOM_DOWN_COLUMN		14

#define GENERATABLE_OBJECT_NAME_COLUMN			0
#define GENERATABLE_OBJECT_DESCRIPTION_COLUMN		1
#define GENERATABLE_OBJECT_ATTACK_POINTS_COLUMN		2
#define GENERATABLE_OBJECT_GENERATE_PROBABILITY_COLUMN	3
#define GENERATABLE_OBJECT_STAMINA_POINTS_COLUMN	4
#define GENERATABLE_OBJECT_MAGIC_POINTS_COLUMN		5
#define GENERATABLE_OBJECT_VERB_COLUMN			7
#define GENERATABLE_OBJECT_VERBMESSAGE_COLUMN		8

#ifndef ROOM_H
#define ROOM_H
typedef struct roomobject {			/* objects */
	unsigned int id;
	char *name[BUF_SIZE];
	char *owner[BUF_SIZE];
	char *description[BUF_SIZE];
	int attackpoints;
	int generateprobability;
	int staminapoints;
	int magicpoints;
	int attributes;
	char *verb[BUF_SIZE];
	char *verbmessage[BUF_SIZE];
	struct roomobject *prev;
	struct roomobject *next;
} roomobject;

typedef struct room {				/* rooms */
	unsigned int id;
	char *name[BUF_SIZE];
	char *owner[BUF_SIZE];
	char *description[BUF_SIZE];
	int attributes;
	int exits[11];
	roomobject *roomobjects;
	roomobject *roomobjects_last;
	int monstercount;
	monster *roommonsters;
	monster *roommonsters_last;
} room;
#endif

