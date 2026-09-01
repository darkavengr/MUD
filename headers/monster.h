#include "size.h"

#define MONSTER_SPAWN_PROB	5
#define ROOM_MONSTER_COUNT	5

#define MONSTER_NAME_COLUMN		0
#define MONSTER_DESCRIPTION_COLUMN	1
#define MONSTER_STAMINA_COLUMN		2
#define MONSTER_EVIL_COLUMN		3
#define MONSTER_ARRIVEMESSAGE_COLUMN	4
#define MONSTER_LEAVEMESSAGE_COLUMN	5
#define MONSTER_CREATEMESSAGE_COLUMN	6
#define MONSTER_DIEMESSAGE_COLUMN	7
#define MONSTER_MOVEODDS_COLUMN		8
#define MONSTER_GENERATEODDS_COLUMN	9
#define MONSTER_DAMAGEDEALT_COLUMN	10
#define MONSTER_EXPERIENCEPOINTS_COLUMN	11
#define MONSTER_MOVEDELAY_COLUMN	12

#ifndef MONSTER_H
#define MONSTER_H

typedef struct monster {			/* monsters */
	unsigned int id;
	char *name[BUF_SIZE];
	char *description[BUF_SIZE];
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

void MoveMonster(void);
int GenerateMonsters(void);
int CopyMonsterToRoom(int room,int destroom,monster *monster);
int LoadMonsters(void);
int AddMonsterToRoom(monster *sourcemonster,int RoomNumber);
monster *FindFirstMonsterInRoom(int RoomNumber);
monster *FindNextMonsterInRoom(monster *previous);
int DeleteMonster(int RoomNumber,unsigned int MonsterID);
int GetRoomMonsterEvil(int RoomNumber,unsigned int MonsterID);
char *GetRoomMonsterName(int RoomNumber,int RoomMonster);
int GetRoomMonsterEvil(int RoomNumber,unsigned int MonsterID);

