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
int MonsterCount=0;

extern room *rooms;

int MoveMonster(void) {
int MoveProbability;
int RoomNumber;
int WhichMonster;
int AttackProbability;
int MoveDirection;

srand(time(NULL));
RoomNumber=1+rand() % GetNumberOfRooms();		/* which room */

if(GetNumberOfMonstersInRoom(RoomNumber) == 0) return(0);	/* no monsters in room */

if(GetNumberOfMonstersInRoom(RoomNumber) > 0) WhichMonster=1+rand() % GetNumberOfMonstersInRoom(RoomNumber);	/* which monster */

/* attack user */
AttackProbability=rand() % (GetRoomMonsterEvil(RoomNumber,WhichMonster) + 1)+1;
if(AttackProbability == 1) AttackUser(RoomNumber,WhichMonster);

/* move monster */

MoveProbability=rand() % (GetRoomMonsterEvil(RoomNumber,WhichMonster) + 1)+1;

if(MoveProbability == 1) {
	MoveDirection=rand() % 11;

	if(rooms[RoomNumber].exits[MoveDirection] != 0 && (rooms[MoveDirection].attr & ROOM_HAVEN) == 0) {
		CopyMonsterToRoom(RoomNumber,rooms[RoomNumber].exits[MoveDirection],WhichMonster);
	}
}

return(0);
}
		
int GenerateMonsters(void) {
monster *monsterlist;
int RandomMonsterNumber;
int RoomNumber;
int MonsterGenerateCount;
int MonsterGenerateLoop;
					
for(RoomNumber=0;RoomNumber<GetNumberOfRooms();RoomNumber++) {
	if((rooms[RoomNumber].attr & ROOM_HAVEN)) continue;		/* skip haven rooms */

	MonsterGenerateCount=rand() % ROOM_MONSTER_COUNT; /* get numbers */

	for(MonsterGenerateLoop=0;MonsterGenerateLoop<MonsterGenerateCount;MonsterGenerateLoop++) {
		/* copy monsters from monster list to monster list in room */

		monsterlist=monsters;
		RandomMonsterNumber=rand() % MonsterCount; /* get numbers */

		AddMonsterToRoom(&monsters[RandomMonsterNumber],RoomNumber);	/* add monster to room */

		SendMessageToAllInRoom(rooms[RoomNumber].room,rooms[RoomNumber].roommonsters_last->createmessage);			      
	}
}

return(0);
}

int CopyMonsterToRoom(int room,int destroom,monster *monster) {
printf("Moving monster %s to %d\n",monster->name,destroom);

SendMessageToAllInRoom(room,monster->leavemessage);

AddMonsterToRoom(monster,destroom);	/* copy monster to room */
//DeleteMonster(room,monsterno);				/* remove monster from source room */


SendMessageToAllInRoom(destroom,monster->arrivemessage);

return(0);
}

int DeleteMonster(int room,int monsterno) {
if(rooms[room].monstercount == 0) return(-1);	/* no monsters */

rooms[room].monstercount--;
return(0);
}

int LoadMonsters(void) {
monster *monsternext;
FILE *handle;
int LineCount;
char *linetokens[10][BUF_SIZE];
char *LineBuffer[BUF_SIZE];
int ErrorCount=0;

MonsterCount=0;

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
			MonsterCount++;
			monsters=realloc(monsters,sizeof(monster)*(MonsterCount+1));
			if(monsters == NULL) {
				perror("\nmud:");
				exit(NOMEM);
			}
		}

		strcpy(monsters[MonsterCount].name,linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"description") == 0) {
		sprintf(monsters[MonsterCount].desc,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"stamina") == 0) {
		monsters[MonsterCount].stamina=atoi(linetokens[1]);	
	}
	else if(strcmp(linetokens[0],"evil") == 0) {
		monsters[MonsterCount].evil=atoi(linetokens[1]);		
	}
	else if(strcmp(linetokens[0],"arrive") == 0) {
		sprintf(monsters[MonsterCount].arrivemessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"die") == 0) {
		sprintf(monsters[MonsterCount].diemessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"damage") == 0) {
		monsters[MonsterCount].damage=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"moveodds") == 0) {
		monsters[MonsterCount].moveodds=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"genodds") == 0) {
		monsters[MonsterCount].genodds=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"leave") == 0) {
		sprintf(monsters[MonsterCount].leavemessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"create") == 0) {
		sprintf(monsters[MonsterCount].createmessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"sleep") == 0) {
		monsters[MonsterCount].sleep=atoi(linetokens[1]);			
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

int AddMonsterToRoom(monster *sourcemonster,int RoomNumber) {
if(rooms[RoomNumber].roommonsters == NULL) {
	rooms[RoomNumber].roommonsters=calloc(1,sizeof(monster));	/* allocate objects */
	if(rooms[RoomNumber].roommonsters == NULL) return(-1);		/* can't allocate */

	rooms[RoomNumber].roommonsters_last=rooms[RoomNumber].roommonsters;
}
else
{
	rooms[RoomNumber].roommonsters_last->next=calloc(1,sizeof(monster));	/* allocate objects */
	if(rooms[RoomNumber].roommonsters_last->next == NULL) return(-1);		/* can't allocate */
	
	rooms[RoomNumber].roommonsters_last=rooms[RoomNumber].roommonsters_last->next;
}

memcpy(rooms[RoomNumber].roommonsters_last,sourcemonster,sizeof(monster));	/* copy monster data */

rooms[RoomNumber].roommonsters_last->next=NULL;

printf("Created monster %s\n",rooms[RoomNumber].roommonsters_last->name);
return(0);
}

monster *FindFirstMonsterInRoom(int RoomNumber) {
return(rooms[RoomNumber].roommonsters);
}

monster *FindNextMonsterInRoom(monster *previous) {
return(previous->next);
}

