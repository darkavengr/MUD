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

#include "defines.h"

char *monrel="/config/monsters.mud";
monster *monsters=NULL;

extern room *rooms;
extern user *users;

int movemonster(void) {
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

getconfigurationinformation(&config);

srand(time(NULL));

 roomnumber=1+rand() % config.roomcount;		/* which room */

 if(rooms[roomnumber].monstercount == 0) return(-1);

 roommonster=rand() % rooms[roomnumber].monstercount;	/* which monster */

 if(rooms[roomnumber].roommonsters[roommonster].room == 0) return(-1); /* no monster */

/*
	 * move monster

 */

/* attack user */
 attack=rand() % (rooms[roomnumber].roommonsters[roommonster].evil + 1)+1;

 if(attack == 1) {
  usernext=users;

  while(usernext != NULL) {
   if(usernext->room == roomnumber) {		/* user is in room */
    sta=rand() % (rooms[roomnumber].roommonsters[roommonster].evil + 1) - 0;		/* random damage */

    sprintf(buf,"%s attacks %s causing %d points of damage\r\n",rooms[roomnumber].roommonsters[roommonster].name,usernext->name,sta);
    sendmudmessagetoall(usernext->handle,roomnumber,buf);

    usernext->staminapoints=usernext->staminapoints-sta;

    updateuser(usernext,usernext->name,"",0,0,"",0,usernext->staminapoints,0,0,"","",0);

   }

    usernext=usernext->next;
  }
}

 move=rand() % 11;
 newroom=rooms[roomnumber].exits[move];

 if(rooms[roomnumber].exits[move] != 0 && (rooms[newroom].attr & ROOM_HAVEN) == 0) {
  copymonstertoroom(roomnumber,rooms[roomnumber].exits[move],roommonster);
 }

return(0);
}
   
int createmonster(void) {
  monster *monsternext;
  monster *monsterlist;
  monster *monsterlast;
  room *roomnext;
 int randx;
 int r;
 int monstercount=0;
 int roomnumber;
 int countx;
 int monsterno;
 int gencount;
 int mlc;
 CONFIG config;

 getconfigurationinformation(&config);

 monsterlist=monsters;
 while(monsterlist != NULL) {
  monstercount++;
  monsterlist=monsterlist->next;
 }
  	  	
for(roomnumber=0;roomnumber<config.roomcount;roomnumber++) {
 if((rooms[roomnumber].attr & ROOM_HAVEN)) continue;		/* skip haven rooms */

 gencount=rand() % (ROOM_MONSTER_COUNT/2); /* get numbers */

 rooms[roomnumber].monstercount=gencount;

 for(countx=0;countx<gencount;countx++) {
/* copy monsters from monster list  to monster list in room */

	  monsterlist=monsters;
          monsterlast=monsterlist;

		    randx=rand() % (monstercount); /* get numbers */

		     mlc=0;


		     monsterlist=monsters;
		     while(monsterlist != NULL) {	/* find monster */
		      if(randx == mlc) break;
                      mlc++;

                      monsterlist=monsterlist->next;
                     }

 		     for(monsterno=0;monsterno<ROOM_MONSTER_COUNT;monsterno++) {	/* find free monster in room */
	              if(rooms[roomnumber].roommonsters[monsterno].room == 0) break;
                     }

                     if(roomnumber >= ROOM_MONSTER_COUNT) break;		/* no free monsters */
	

    		      memcpy(&rooms[roomnumber].roommonsters[monsterno],monsterlist,sizeof( monster));	/* copy monster data */
			          
		        rooms[roomnumber].roommonsters[monsterno].roomptr=roomnext;		/* pointer  to room */
		        rooms[roomnumber].roommonsters[monsterno].room=rooms[roomnumber].room;

			sendmudmessagetoall(rooms[roomnumber].room,rooms[roomnumber].roommonsters[monsterno].createmessage);	        
    }
  }


}

int copymonstertoroom(int room,int destroom,int monsterno) {
int countx;

for(countx=0;countx !=ROOM_MONSTER_COUNT;countx++) {	/* find free monster in room */
 if(rooms[destroom].roommonsters[countx].room == 0) break;
}

printf("Moving monster %s to %d\n",rooms[room].roommonsters[monsterno].name,destroom);

if(countx == ROOM_MONSTER_COUNT) return(-1);		/* no free monsters */

sendmudmessagetoall(room,rooms[room].roommonsters[monsterno].leavemessage);

memcpy(&rooms[destroom].roommonsters[countx],&rooms[room].roommonsters[monsterno],sizeof( monster));	/* copy monster data */
deletemonster(room,monsterno);

sendmudmessagetoall(destroom,rooms[destroom].roommonsters[monsterno].arrivemessage);

return(0);
}

int deletemonster(int room,int monsterno) {
int countx;
monster *montemp[ROOM_MONSTER_COUNT];

if(rooms[room].monstercount == 0) return(-1);	/* no monsters */

/* remove last monster */
if(monsterno == ROOM_MONSTER_COUNT) {
 memset(&rooms[room].roommonsters[monsterno],0,sizeof(room));

 rooms[room].monstercount--;
 return(0);
}

countx=(ROOM_MONSTER_COUNT-monsterno);		/* number of remaining monsters */

memcpy(montemp,&rooms[room].roommonsters[monsterno+1],countx);
memcpy(&rooms[room].roommonsters[monsterno],montemp,countx);
rooms[room].monstercount--;

return(0);
}

int loadmonsters(void) {
monster *monsternext;
FILE *handle;
char *b;
char c;
int lc;
char *ab[10][BUF_SIZE];
char *z[BUF_SIZE];
int errorcount=0;
char *monsterconf[BUF_SIZE];

getcwd(monsterconf,BUF_SIZE);
strcat(monsterconf,monrel);

monsternext=monsters;

 handle=fopen(monsterconf,"rb");
 if(handle == NULL) {                                           /* couldn't open file */
  printf("\nmud: Can't open configuration file %s\n",monsterconf);
  exit(NOCONFIGFILE);
 }

 while(!feof(handle)) {
  fgets(z,BUF_SIZE,handle);		/* get and parse line */

  if(feof(handle)) break;		/* return if at end */

//  if(strlen(z) == 1) continue;

  b=z;
  b=b+strlen(z);
  b--;

  if(*b == '\n') *b=0;
  b--;
  if(*b == '\r') *b=0;

  lc++;

  b=z;
  c=*b;
  if(c == '#')  continue;		/* skip comments */
  if(c == '\n')  continue;		/* skip newline */


  tokenize_line(z,ab,":\n");				/* tokenize line */

  if(strcmp(ab[0],"begin_monster") == 0) {	/* end */
   if(monsters == NULL) {			/* first monster */
    monsters=calloc(1,sizeof(monster));
    if(monsters == NULL) {
     perror("\nmud:");
     exit(NOMEM);
    }

    monsternext=monsters;
   }
   else
   {
    monsternext->next=calloc(1,sizeof(monster));
    monsternext=monsternext->next;

    if(monsternext == NULL) {
     perror("\nmud:");
     exit(NOMEM);
    }
    
   }

   strcpy(monsternext->name,ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"description") == 0) {
   sprintf(monsternext->desc,"%s\r\n",ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"stamina") == 0) {
   monsternext->stamina=atoi(ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"evil") == 0) {
   monsternext->evil=atoi(ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"arrive") == 0) {
   sprintf(monsternext->arrivemessage,"%s\r\n",ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"die") == 0) {
   sprintf(monsternext->diemessage,"%s\r\n",ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"damage") == 0) {
   monsternext->damage=atoi(ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"moveodds") == 0) {
   monsternext->moveodds=atoi(ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"genodds") == 0) {
   monsternext->genodds=atoi(ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"leave") == 0) {
   sprintf(monsternext->leavemessage,"%s\r\n",ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"create") == 0) {
   sprintf(monsternext->createmessage,"%s\r\n",ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"sleep") == 0) {
   monsternext->sleep=atoi(ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"end") == 0) continue;

  printf("\nmud: %d: unknown configuration option %s in %s\n",lc,ab[0],monsterconf);		/* unknown configuration option */
  errorcount++;
}

fclose(handle); 
return(errorcount);
}

