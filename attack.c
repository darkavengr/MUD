/*
  Adventure MUD server 
 
  (c) Copyright Matthew Boote 2018, All rights reserved blah blah blah etc etc etc 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

/* combat functions */

#include <stdio.h>
     #include <fcntl.h>
#ifdef _linux_
 #include <netdb.h>
 #include <sys/socket.h>
 #include <sys/un.h>
 #include <sys/uio.h>
 #include <sys/types.h> 
 #include <netinet/in.h>
 #include <sys/types.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <sys/stat.h>
#endif

#ifdef _WIN32
 #include <winsock2.h>
#endif

#include "defs.h"

extern char *nouser;
extern user *users;

char *fightprompt="fight>";
char *havekilled=" and have killed it\r\n";
char *noponpkilling="Player vs player combat not allowed\r\n";

int attack(user *currentuser,char *target) {
 int count;
 int countx;
 int found;
 int stamina;
 char *b;
 char *cmd[BUF_SIZE];
 int sta;
 int ipos;
 int result;
 user *usernext;
 room *currentroom;
 race *racenext;
 char *buf[BUF_SIZE];

 currentroom=currentuser->roomptr;
 racenext=currentuser->race;

/*
 * can't attack in haven rooms or attack wizards
 */
 if((currentroom->attr & ROOM_HAVEN) && currentuser->status < WIZARD) {
  send(currentuser->handle,notthere,strlen(notthere),0);
  return;
 }

while(currentuser->staminapoints > 0) {		/* until dead */
 send(currentuser->handle,fightprompt,strlen(fightprompt),0);	/* display prompt */

 recv(currentuser->handle,cmd,1,0);		/* get command */

 if(strcmp(cmd,"f") == 0) {		/* if fleeing */
  sprintf(cmd,"%s has fled from battle!\r\n",currentuser->name);	/* send message */
  sendmudmessagetoall(currentuser->room,cmd);

  return;
 }

  
/* find user */
 usernext=users;
 while(usernext != NULL) {		/* find user if not monster */
  if(regexp(usernext->name,target) == TRUE && usernext->loggedin == TRUE) {	/* if object matches */

   if(allowplayerkilling == FALSE) {	/* no player on player killing */
    sprintf(buf,"can't attack %s: no player on player killing allowed\r\n",usernext->name);
    send(currentuser->handle,buf,strlen(buf),0);
    return;
   }

  /* user one attacks user two */
   sta=rand() % (racenext->strength + 1) - 0;		/* random damage */

   if(currentuser->status >= WIZARD) {
    sprintf(buf,"%s tries to attack %s but it just bounces off\r\n",usernext->next,currentuser->name);
    sendmudmessagetoall(currentuser->room,buf);
    continue;
   }
 
   sprintf(buf,"%s attacks %s causing %d points of damage\r\n"	,currentuser->name,usernext->name,sta);
   sendmudmessagetoall(currentuser->room,buf);

   usernext->staminapoints=usernext->staminapoints-sta;
   updateuser(currentuser,usernext->name,"",0,0,"",0,usernext->staminapoints,0,0,"","",0);
  

 /* user two attacks user one */

   if(usernext->status >= WIZARD) {
    sprintf(buf,"%s tries to attack %s but it just bounces off\r\n",currentuser->name,usernext->name);
    sendmudmessagetoall(currentuser->room,buf);
    continue;
   }

   sta=rand() % (racenext->strength + 1) - 0;		/* random damage */
 
   sprintf(buf,"%s attacks %s causing %d points of damage\r\n",usernext->name,currentuser->name,sta);
   sendmudmessagetoall(currentuser->room,buf);

   currentuser->staminapoints=currentuser->staminapoints-sta;
   updateuser(currentuser,currentuser->name,"",0,0,"",0,usernext->staminapoints,0,0,"","",0);
  }  

  usernext=usernext->next;
 }

for(count=0;count<ROOM_MONSTER_COUNT;count++) {
  if(regexp(rooms[currentroom->room].roommonsters[count].name,target) == TRUE) {	/* if object matches */
   /* find monster */
  /* monster attacks user */
   sta=rand() % (rooms[currentroom->room].roommonsters[count].damage + 1) - 0;		/* random damage */

   sprintf(buf,"%s attacks %s causing %d points of damage\r\n",rooms[currentroom->room].roommonsters[count].name,currentuser->name,sta);
   sendmudmessagetoall(currentuser->room,buf);

   usernext->staminapoints=usernext->staminapoints-sta;

   updateuser(currentuser,currentuser->name,"",0,0,"",0,usernext->staminapoints,0,0,"","",0);

 /* user attacks monster */

   sta=rand() % (racenext->strength + 1) - 0;	/* random damage */

   sprintf(buf,"%s attacks %s causing %d points of damage\r\n",currentuser->next,rooms[currentroom->room].roommonsters[count].name,sta);
   sendmudmessagetoall(currentuser->room,buf);

   rooms[currentroom->room].roommonsters[count].stamina -= sta;
  }

 }

}

if(found == FALSE)  send(currentuser->handle,nouser,strlen(nouser),0);		/* missing player */
return;
}
 
