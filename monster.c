/*
* create monsters */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sqlite3.h>

#ifdef __linux__
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
#include "winsock.h"
#endif

#include "errors.h"
#include "user.h"
#include "room.h"
#include "monster.h"
#include "config.h"
#include "getconfig.h"
#include "world.h"

int NumberOfMonsterTypes=0;

void MoveMonster(void) {
int MoveProbability;
int RoomNumber;
int WhichMonster;
int AttackProbability;
int MoveDirection;
monster *MonsterPtr;
int MonsterCount=0;
char *columnptr;

srand(time(NULL));

if(GetNumberOfRooms() == 0) return;

RoomNumber=1+rand() % (GetNumberOfRooms() -1);		/* which room */

//printf("random room=%d\n",RoomNumber);

if(GetNumberOfMonstersInRoom(RoomNumber) == 0) return;	/* no monsters in room */

//printf("number of monsters in room %d=%d\n",RoomNumber,GetNumberOfMonstersInRoom(RoomNumber));

WhichMonster=rand() % GetNumberOfMonstersInRoom(RoomNumber);	/* which monster */

//printf("Move monster 1\n");

//printf("WhichMonster=%d\n",WhichMonster);

/* find monster by index */

MonsterPtr=FindFirstMonsterInRoom(RoomNumber);

//printf("first monster=%lX\n",MonsterPtr);

while(MonsterPtr != NULL) {
	if(MonsterCount++ == WhichMonster) {
		//printf("Move monster 2\n");
		//printf("MonsterPtr=%lX\n",MonsterPtr);

		AttackProbability=rand() % MonsterPtr->evil;

		//printf("Monster attack probability=%d/%d\n",AttackProbability,MonsterPtr->evil);

		if(AttackProbability == 1) AttackUser(RoomNumber,WhichMonster);
		/* move monster */

		MoveProbability=rand() % (MonsterPtr->moveodds + 1)+1;

		//printf("Monster move probability=%d/%d\n",MoveProbability,MonsterPtr->moveodds);

		//printf("Move monster 3\n");

		if(MoveProbability == 1) {
			do {
				MoveDirection=rand() % NUMBER_OF_DIRECTIONS;

				//printf("Move monster 3.1\n");

				if(GetRoomExit(RoomNumber,MoveDirection) != 0 && (GetRoomAttributes(RoomNumber) & ROOM_HAVEN) == 0) {
					//printf("Move monster 3.2\n");

					CopyMonsterToRoom(RoomNumber,GetRoomExit(RoomNumber,MoveDirection),MonsterPtr);
					break;
				}

			} while(GetRoomExit(RoomNumber,MoveDirection) != 0 && (GetRoomAttributes(RoomNumber) & ROOM_HAVEN) == 0);

		}

		//printf("Monster moved\n");
	}

	//printf("MonsterPtr=%lX\n",MonsterPtr);

	MonsterPtr=FindNextMonsterInRoom(MonsterPtr);
}

}
		
int GenerateMonsters(void) {
monster tempmonster;
unsigned int RandomMonsterNumber;
int RoomNumber;
sqlite3_stmt *SQLStatementHandle;
int returncode;
int MonsterCount;
char *columnptr;

for(RoomNumber=1;RoomNumber < GetNumberOfRooms();RoomNumber++) {
	
	if((GetRoomAttributes(RoomNumber) & ROOM_HAVEN)) continue;		/* skip haven rooms */

	MonsterCount=rand() % ROOM_MONSTER_COUNT; /* get number of monsters to generate */

	/* prepare SQL statement */

	if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM MONSTERS ORDER BY RANDOM() LIMIT ?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
		//printf("mud: %s\n",sqlite3_errmsg(GetDatabaseHandle()));
		return(-1);
	}	

	sqlite3_bind_int(SQLStatementHandle,1,MonsterCount);	/* bind parameter */

	/* copy monster to room */

	while(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {
		columnptr=sqlite3_column_text(SQLStatementHandle,MONSTER_NAME_COLUMN);
		if(columnptr != NULL) strncpy(tempmonster.name,columnptr,BUF_SIZE);

		columnptr=sqlite3_column_text(SQLStatementHandle,MONSTER_DESCRIPTION_COLUMN);
		if(columnptr != NULL) strncpy(tempmonster.description,columnptr,BUF_SIZE);

		tempmonster.stamina=sqlite3_column_int(SQLStatementHandle,MONSTER_STAMINA_COLUMN);
		tempmonster.evil=sqlite3_column_int(SQLStatementHandle,MONSTER_EVIL_COLUMN);

		columnptr=sqlite3_column_text(SQLStatementHandle,MONSTER_ARRIVEMESSAGE_COLUMN);
		if(columnptr != NULL) strncpy(tempmonster.arrivemessage,columnptr,BUF_SIZE);

		sqlite3_column_text(SQLStatementHandle,MONSTER_LEAVEMESSAGE_COLUMN);
		if(columnptr != NULL) strncpy(tempmonster.leavemessage,columnptr,BUF_SIZE);

		columnptr=sqlite3_column_text(SQLStatementHandle,MONSTER_CREATEMESSAGE_COLUMN);
		if(columnptr != NULL) strncpy(tempmonster.createmessage,columnptr,BUF_SIZE);

		columnptr=sqlite3_column_text(SQLStatementHandle,MONSTER_DIEMESSAGE_COLUMN);
		if(columnptr != NULL) strncpy(tempmonster.diemessage,columnptr,BUF_SIZE);

		tempmonster.moveodds=sqlite3_column_int(SQLStatementHandle,MONSTER_MOVEODDS_COLUMN);
		tempmonster.genodds=sqlite3_column_int(SQLStatementHandle,MONSTER_GENERATEODDS_COLUMN);
		tempmonster.damage=sqlite3_column_int(SQLStatementHandle,MONSTER_DAMAGEDEALT_COLUMN);
		tempmonster.experiencepoints=sqlite3_column_int(SQLStatementHandle,MONSTER_EXPERIENCEPOINTS_COLUMN);
		tempmonster.sleep=sqlite3_column_int(SQLStatementHandle,MONSTER_MOVEDELAY_COLUMN);

		AddMonsterToRoom(&tempmonster,RoomNumber);		/* add monster to room */

		SetNumberOfMonstersInRoom(RoomNumber,MonsterCount);

		SendMessageToAllInRoom(RoomNumber,tempmonster.createmessage);

	}

	sqlite3_finalize(SQLStatementHandle);
}

return(0);
}

int CopyMonsterToRoom(int SourceRoom,int DestinationRoom,monster *monster) {
int NumberOfMonstersInRoom;

SendMessageToAllInRoom(SourceRoom,monster->leavemessage);

AddMonsterToRoom(monster,DestinationRoom);	/* copy monster to room */

NumberOfMonstersInRoom=GetNumberOfMonstersInRoom(SourceRoom);
SetNumberOfMonstersInRoom(SourceRoom,++NumberOfMonstersInRoom);	/* increment number of monsters in destination room */

NumberOfMonstersInRoom=GetNumberOfMonstersInRoom(DestinationRoom);
SetNumberOfMonstersInRoom(DestinationRoom,--NumberOfMonstersInRoom);	/* decrement number of monsters in destination room */

//DeleteMonster(room,monsterno);				/* remove monster from source room */

SendMessageToAllInRoom(DestinationRoom,monster->arrivemessage);

return(0);
}

int DeleteMonster(int RoomNumber,unsigned int MonsterID) {
monster *MonsterPtr;
monster *last;

MonsterPtr=FindFirstMonsterInRoom(RoomNumber);

while(MonsterPtr != NULL) {
	last=MonsterPtr;

	if(MonsterPtr->id == MonsterID) {		/* found monster */

		if(MonsterPtr == FindFirstMonsterInRoom(RoomNumber)) {		/* first in list */
			SetFirstMonsterInRoom(RoomNumber,MonsterPtr->next);
		}
		else if(MonsterPtr->next == NULL) {		/* last in list */
			last->next=NULL;
		}
		else
		{
			last->next=MonsterPtr->next;
		}

		return(0);
	}

	MonsterPtr=FindNextMonsterInRoom(MonsterPtr);
}

return(-1);
}

int GetRoomMonsterEvil(int RoomNumber,unsigned int MonsterID) {
monster *MonsterPtr;

MonsterPtr=FindFirstMonsterInRoom(RoomNumber);

while(MonsterPtr != NULL) {
	if(MonsterPtr->id == MonsterID) return(MonsterPtr->evil);		/* found monster */

	MonsterPtr=FindNextMonsterInRoom(MonsterPtr);
}

return(-1);
}

char *GetRoomMonsterName(int RoomNumber,int MonsterID) {
monster *MonsterPtr;

MonsterPtr=FindFirstMonsterInRoom(RoomNumber);

while(MonsterPtr != NULL) {
	if(MonsterPtr->id == MonsterID) return(MonsterPtr->name);		/* found monster */

	MonsterPtr=FindNextMonsterInRoom(MonsterPtr);
}

return(-1);
}

