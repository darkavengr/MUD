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

#include "defines.h"

extern user *users;
extern room *rooms;

int attack(user *currentuser,char *target) {
 int count;
 int countx;
 int found;
 char *b;
 int sta;
 user *usernext;
 room *currentroom;
 race *racenext;
 char *buf[BUF_SIZE];
 CONFIG config;
 int savestamina;

 currentroom=currentuser->roomptr;
 racenext=currentuser->race;

 getconfigurationinformation(&config);

/*
 * can't attack in haven rooms or attack wizards
 */
 if((currentroom->attr & ROOM_HAVEN) && currentuser->status < WIZARD) {
  display_error(count,ATTACK_HAVEN);
  return;
}

/* find user */
 usernext=users;

while(usernext != NULL) {		/* find user  */

 if(strcmp(usernext->name,target) == 0 && usernext->loggedin == TRUE) {
  found=TRUE;
  break;	/* if object matches */
 }

 usernext=usernext->next;
}

if(found == TRUE) {
 if(config.allowplayerkilling == FALSE) {	/* no player on player killing */
  sprintf(buf,"Can't attack %s because player versus player combat is not allowed\r\n",usernext->name);
  send(currentuser->handle,buf,strlen(buf),0);
  return;
 }

 while(usernext->staminapoints > 0 && currentuser->staminapoints > 0) {

  /* user one attacks user two */
   sta=rand() % (racenext->strength + 1) - 0;		/* random damage */
   
   if(usernext->status >= WIZARD) {
   sprintf(buf,"%s tries to attack %s but it just bounces off\r\n",usernext->next,currentuser->name);
   sendmudmessagetoall(currentuser->room,buf);
   continue;
  }
  else
  {
   sprintf(buf,"%s attacks %s causing %d points of damage\r\n",currentuser->name,usernext->name,sta);
   sendmudmessagetoall(usernext->room,buf);

   usernext->staminapoints -= sta;
   currentuser->staminapoints += sta;
   currentuser->experiencepoints += (sta*currentuser->status);

   updateuser(currentuser,usernext->name,"",0,0,"",0,usernext->staminapoints,0,0,"","",0);
   updateuser(currentuser,usernext->name,"",0,0,"",0,currentuser->staminapoints,currentuser->experiencepoints,0,"","",0);
  }

 /* user two attacks user one */

   if(currentuser->status >= WIZARD) {
    sprintf(buf,"%s tries to attack %s but it just bounces off\r\n",currentuser->name,usernext->name);
    sendmudmessagetoall(currentuser->room,buf);
   }
   else
   {
    sta=rand() % (racenext->strength + 1) - 0;		/* random damage */
 
    sprintf(buf,"%s attacks %s causing %d points of damage\r\n",usernext->name,currentuser->name,sta);
    sendmudmessagetoall(currentuser->room,buf);

   currentuser->staminapoints -= sta;
   currentuser->experiencepoints += (sta*currentuser->status);

   updateuser(currentuser,usernext->name,"",0,0,"",0,usernext->staminapoints,0,0,"","",0);
   updateuser(currentuser,usernext->name,"",0,0,"",0,currentuser->staminapoints,currentuser->experiencepoints,0,"","",0);

   }

 }
}
	
for(count=0;count != rooms[currentroom->room].monstercount;count++) {

  if(strcmp(rooms[currentroom->room].roommonsters[count].name,target) == 0) {	/* if object matches */
   found=TRUE;

   savestamina=rooms[currentroom->room].roommonsters[count].stamina;

   while(rooms[currentroom->room].roommonsters[count].stamina > 0 && currentuser->staminapoints > 0) {
    /* player attacks monster */

    sta=rand() % (racenext->strength + 1) - 0;		/* random damage */
    
    sprintf(buf,"%s attacks %s causing %d points of damage\r\n",currentuser->name,rooms[currentroom->room].roommonsters[count].name,sta);
    sendmudmessagetoall(currentuser->room,buf);

    rooms[currentroom->room].roommonsters[count].stamina -= sta;

    if(rooms[currentroom->room].roommonsters[count].stamina <= 0) { /* monster defeated */
     sprintf(buf,"%s has killed the %s!\r\n",currentuser->name,rooms[currentroom->room].roommonsters[count].name);
     sendmudmessagetoall(currentuser->room,buf);

     sprintf(buf,"You have gained %d stamina points\r\n",(savestamina*currentuser->status));
     send(currentuser->handle,buf,strlen(buf),0);

     currentuser->staminapoints += savestamina;

     sprintf(buf,"You have gained %d experience points\r\n",(savestamina*currentuser->status));
     send(currentuser->handle,buf,strlen(buf),0);

     currentuser->experiencepoints += (savestamina*currentuser->status);

     deletemonster(currentuser->room,count);
     return;
    }

    /* monster attacks user */
    sta=rand() % (rooms[currentroom->room].roommonsters[count].damage + 1) - 0;	/* random damage */

    currentuser->staminapoints -= sta;

    updateuser(currentuser,currentuser->name,"",0,0,"",0,currentuser->staminapoints,0,0,"","",0);

    sprintf(buf,"%s attacks %s causing %d points of damage\r\n",rooms[currentroom->room].roommonsters[count].name,currentuser->name,sta);
    sendmudmessagetoall(currentuser->room,buf);
  }

  break;
 }
}

if(found == FALSE) display_error(count,UNKNOWN_USER);		/* missing player */
return;
}
 
