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

char *monrel="/config/monsters.mud";
monster *monsters=NULL;
int monstercount=0;

extern room *rooms;

int MoveMonster(void) {
user *usernext;
char *buf[BUF_SIZE];
int move;
int damage;
int roomnumber;
int roommonster;
int attack;
int sta;
int newroom;
CONFIG config;

GetConfigurationInformation(&config);

srand(time(NULL));
roomnumber=1+rand() % GetNumberOfRooms();		/* which room */

if(GetNumberOfMonstersInRoom(roomnumber) == 0) return(0);	/* no monsters in room */

if(GetNumberOfMonstersInRoom(roomnumber) > 0) roommonster=1+rand() % GetNumberOfMonstersInRoom(roomnumber);	/* which monster */

/* attack user */
attack=rand() % (GetRoomMonsterEvil(roomnumber,roommonster) + 1)+1;
if(attack == 1) AttackUser(roomnumber,roommonster);

move=rand() % 11;
newroom=rooms[roomnumber].exits[move];

/* move monster */
if(rooms[roomnumber].exits[move] != 0 && (rooms[newroom].attr & ROOM_HAVEN) == 0) {
	CopyMonsterToRoom(roomnumber,rooms[roomnumber].exits[move],roommonster);
}

return(0);
}
		
int CreateMonster(void) {
monster *monsterlist;
int randx;
int r;
int roomnumber;
int countx;
int monsterno;
int gencount;
CONFIG config;

GetConfigurationInformation(&config);
					
for(roomnumber=0;roomnumber<config.roomcount;roomnumber++) {
	if((rooms[roomnumber].attr & ROOM_HAVEN)) continue;		/* skip haven rooms */

	gencount=rand() % ROOM_MONSTER_COUNT; /* get numbers */

	for(countx=0;countx<gencount;countx++) {
		/* copy monsters from monster list to monster list in room */

		monsterlist=monsters;
		randx=rand() % monstercount; /* get numbers */

		AddMonsterToRoom(&monsters[randx],roomnumber);	/* add monster to room */

		SendMessageToAllInRoom(rooms[roomnumber].room,rooms[roomnumber].roommonsters_last->createmessage);			      
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
int countx;
monster *montemp[ROOM_MONSTER_COUNT];

if(rooms[room].monstercount == 0) return(-1);	/* no monsters */

rooms[room].monstercount--;
return(0);
}

int LoadMonsters(void) {
monster *monsternext;
FILE *handle;
int lc;
char *linetokens[10][BUF_SIZE];
char *linebuffer[BUF_SIZE];
int errorcount=0;
char *monsterconf[BUF_SIZE];

monstercount=0;

getcwd(linebuffer,BUF_SIZE);

sprintf(monsterconf,"%s%s",linebuffer,monrel);		/* get absolute path of configuration file */

monsternext=monsters;

handle=fopen(monsterconf,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",monsterconf);
	exit(NOCONFIGFILE);
}

while(!feof(handle)) {
	fgets(linebuffer,BUF_SIZE,handle);		/* get and parse line */

	if(feof(handle)) break;		/* return if at end */

	if((char) *linebuffer == '#')  continue;		/* skip comments */
	if((char) *linebuffer == '\n')  continue;		/* skip newline */

	RemoveNewLine(linebuffer);		/* remove newline character */

	TokenizeLine(linebuffer,linetokens,":\n");				/* tokenize line */

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
			monstercount++;
			monsters=realloc(monsters,sizeof(monster)*(monstercount+1));
			if(monsters == NULL) {
				perror("\nmud:");
				exit(NOMEM);
			}
		}

		strcpy(monsters[monstercount].name,linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"description") == 0) {
		sprintf(monsters[monstercount].desc,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"stamina") == 0) {
		monsters[monstercount].stamina=atoi(linetokens[1]);	
	}
	else if(strcmp(linetokens[0],"evil") == 0) {
		monsters[monstercount].evil=atoi(linetokens[1]);		
	}
	else if(strcmp(linetokens[0],"arrive") == 0) {
		sprintf(monsters[monstercount].arrivemessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"die") == 0) {
		sprintf(monsters[monstercount].diemessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"damage") == 0) {
		monsters[monstercount].damage=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"moveodds") == 0) {
		monsters[monstercount].moveodds=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"genodds") == 0) {
		monsters[monstercount].genodds=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"leave") == 0) {
		sprintf(monsters[monstercount].leavemessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"create") == 0) {
		sprintf(monsters[monstercount].createmessage,"%s\r\n",linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"sleep") == 0) {
		monsters[monstercount].sleep=atoi(linetokens[1]);			
	}
	else if(strcmp(linetokens[0],"end") == 0) {
		;;
	}
	else
	{
		printf("\nmud: %d: Unknown configuration option %s in %s\n",lc,linetokens[0],monsterconf);		/* unknown configuration option */
		errorcount++;
	}
}

fclose(handle); 

return(errorcount);
}

int AddMonsterToRoom(monster *sourcemonster,int roomnumber) {
if(rooms[roomnumber].roommonsters == NULL) {
	rooms[roomnumber].roommonsters=calloc(1,sizeof(monster));	/* allocate objects */
	if(rooms[roomnumber].roommonsters == NULL) return(-1);		/* can't allocate */

	rooms[roomnumber].roommonsters_last=rooms[roomnumber].roommonsters;
}
else
{
	rooms[roomnumber].roommonsters_last->next=calloc(1,sizeof(monster));	/* allocate objects */
	if(rooms[roomnumber].roommonsters_last->next == NULL) return(-1);		/* can't allocate */
	
	rooms[roomnumber].roommonsters_last=rooms[roomnumber].roommonsters_last->next;
}

memcpy(rooms[roomnumber].roommonsters_last,sourcemonster,sizeof(monster));	/* copy monster data */

return(0);
}

monster *FindFirstMonsterInRoom(int roomnumber) {
return(rooms[roomnumber].roommonsters);
}

monster *FindNextMonsterInRoom(monster *previous) {
return(previous->next);
}

