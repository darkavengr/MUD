/*
* create monsters */

#include <stdio.h>
#include <string.h>
#include <errno.h>

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

char *MonstersConfigurationFile="config/monsters.mud";
monster *monsters=NULL;
int NumberOfMonsterTypes=0;

void MoveMonster(void) {
int MoveProbability;
int RoomNumber;
int WhichMonster;
int AttackProbability;
int MoveDirection;
monster *MonsterPtr;
int MonsterCount=0;

srand(time(NULL));

if(GetNumberOfRooms() == 0) return;

RoomNumber=1+rand() % GetNumberOfRooms();		/* which room */

printf("random room=%d\n",RoomNumber);
printf("number of monsters in room %d=%d\n",RoomNumber,GetNumberOfMonstersInRoom());

if(GetNumberOfMonstersInRoom(RoomNumber) == 0) return;	/* no monsters in room */

WhichMonster=rand() % GetNumberOfMonstersInRoom(RoomNumber);	/* which monster */

printf("Move monster 1\n");

printf("WhichMonster=%d\n",WhichMonster);

/* find monster by index */

MonsterPtr=FindFirstMonsterInRoom(RoomNumber);

while(MonsterPtr != NULL) {
	if(MonsterCount++ == WhichMonster) break;

	MonsterPtr=FindNextMonsterInRoom(MonsterPtr);
}

printf("Move monster 2\n");

printf("MonsterPtr=%lX\n",MonsterPtr);

AttackProbability=(rand() % MonsterPtr->evil + 1)+1;
if(AttackProbability == 1) AttackUser(RoomNumber,WhichMonster);

/* move monster */

MoveProbability=rand() % (MonsterPtr->moveodds + 1)+1;

printf("Move monster 3\n");

if(MoveProbability == 1) {
	do {
		MoveDirection=rand() % NUMBER_OF_DIRECTIONS;

		printf("Move monster 3.1\n");

		if(GetRoomExit(RoomNumber,MoveDirection) != 0 && (GetRoomAttributes(RoomNumber) & ROOM_HAVEN) == 0) {
			printf("Move monster 3.2\n");

			CopyMonsterToRoom(RoomNumber,GetRoomExit(RoomNumber,MoveDirection),WhichMonster);
			break;
		}

	} while(GetRoomExit(RoomNumber,MoveDirection) != 0 && (GetRoomAttributes(RoomNumber) & ROOM_HAVEN) == 0);

}
printf("Monster moved\n");
return;
}
		
int GenerateMonsters(void) {
monster *monsterlist;
unsigned int RandomMonsterNumber;
int RoomNumber;
int MonsterGenerateCount;
int MonsterGenerateLoop;
int NumberOfMonstersInRoom;
					
for(RoomNumber=0;RoomNumber<GetNumberOfRooms();RoomNumber++) {
	if((GetRoomAttributes(RoomNumber) & ROOM_HAVEN)) continue;		/* skip haven rooms */

	MonsterGenerateCount=rand() % ROOM_MONSTER_COUNT; /* get numbers */

	for(MonsterGenerateLoop=0;MonsterGenerateLoop<MonsterGenerateCount;MonsterGenerateLoop++) {
		/* copy monsters from monster list to monster list in room */

		monsterlist=monsters;
		RandomMonsterNumber=rand() % NumberOfMonsterTypes; /* get numbers */

		AddMonsterToRoom(&monsters[RandomMonsterNumber],RoomNumber);	/* add monster to room */

		NumberOfMonstersInRoom=GetNumberOfMonstersInRoom(RoomNumber);
		SetNumberOfMonstersInRoom(RoomNumber,++NumberOfMonstersInRoom);	/* increment number of monsters in destination room */

		SendMessageToAllInRoom(RoomNumber,monsters[RandomMonsterNumber].createmessage);			      
	}
}

return(0);
}

int CopyMonsterToRoom(int SourceRoom,int DestinationRoom,monster *monster) {
int NumberOfMonstersInRoom;

printf("copy monster=%lX\n",monster);
printf("Moving monster %s to %d\n",monster->name,DestinationRoom);

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

int LoadMonsters(void) {
monster *monsternext;
FILE *handle;
int LineCount;
char *linetokens[10][BUF_SIZE];
char *LineBuffer[BUF_SIZE];
int ErrorCount=0;

NumberOfMonsterTypes=0;

monsternext=monsters;

handle=fopen(MonstersConfigurationFile,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",MonstersConfigurationFile);
	exit(NOCONFIGFILE);
}

while(!feof(handle)) {
	fgets(LineBuffer,BUF_SIZE,handle);		/* get and parse line */

	if(feof(handle)) break;		/* return if at end */

	if((char) *LineBuffer == '#')  continue;		/* skip comments */
	if((char) *LineBuffer == '\n')  continue;		/* skip newline */

	RemoveNewLine(LineBuffer);		/* remove newline character */

	TokenizeLine(LineBuffer,linetokens,":\n");				/* tokenize line */

	if(strcmp(linetokens[0],"begin_monster") == 0) {
		/* monster list is an array, not a linked list because it is randomly accessed */

		if(monsters == NULL) {			/* first monster */
			monsters=calloc(1,sizeof(monster));
			if(monsters == NULL) {
				perror("\nmud:");
				exit(NOMEM);
			}
		}
		else
		{
			NumberOfMonsterTypes++;
			monsters=realloc(monsters,sizeof(monster)*(NumberOfMonsterTypes+1));
			if(monsters == NULL) {
				perror("\nmud:");
				exit(NOMEM);
			}
		}

		strcpy(monsters[NumberOfMonsterTypes].name,linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"description") == 0) {
		sprintf(monsters[NumberOfMonsterTypes].desc,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"stamina") == 0) {
		monsters[NumberOfMonsterTypes].stamina=atoi(linetokens[1]);	
	}
	else if(strcmp(linetokens[0],"evil") == 0) {
		monsters[NumberOfMonsterTypes].evil=atoi(linetokens[1]);		
	}
	else if(strcmp(linetokens[0],"arrive") == 0) {
		sprintf(monsters[NumberOfMonsterTypes].arrivemessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"die") == 0) {
		sprintf(monsters[NumberOfMonsterTypes].diemessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"damage") == 0) {
		monsters[NumberOfMonsterTypes].damage=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"moveodds") == 0) {
		monsters[NumberOfMonsterTypes].moveodds=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"genodds") == 0) {
		monsters[NumberOfMonsterTypes].genodds=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"leave") == 0) {
		sprintf(monsters[NumberOfMonsterTypes].leavemessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"create") == 0) {
		sprintf(monsters[NumberOfMonsterTypes].createmessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"sleep") == 0) {
		monsters[NumberOfMonsterTypes].sleep=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"end") == 0) {
		;;
	}
	else
	{
		printf("\nmud: %d: Unknown configuration option %s in %s\n",LineCount,linetokens[0],MonstersConfigurationFile);		/* unknown configuration option */
		ErrorCount++;
	}
}

fclose(handle); 

return(ErrorCount);
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

printf("ID=%d\n",MonsterID);

while(MonsterPtr != NULL) {
	if(MonsterPtr->id == MonsterID) {
		printf("name=%s\n",MonsterPtr->name);
		return(MonsterPtr->evil);		/* found monster */
	}

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

