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

#include "bool.h"
#include "attack.h"
#include "race.h"
#include "errors.h"
#include "user.h"
#include "monster.h"
#include "config.h"
#include "getconfig.h"

extern room *rooms;

int attack(user *currentuser,char *target) {
int MonsterLoopCount;
int found;
char *b;
int HitPoints;
user *usernext;
room *currentroom;
race *racenext;
char *OutputMessage[BUF_SIZE];
CONFIG config;
int SaveStamina;

currentroom=currentuser->roomptr;
racenext=currentuser->race;

GetConfigurationInformation(&config);

/*
* can't attack in haven rooms or attack wizards
*/
if((currentroom->attr & ROOM_HAVEN) && currentuser->status < WIZARD) {
	SetLastError(currentuser,ATTACK_HAVEN);
	return(-1);
}

/* find user */
usernext=GetUserPointerByName(target);		/* find user */
if(usernext != NULL) found=TRUE;			/* found user */

if(found == TRUE) {
	if(config.allowplayerkilling == FALSE) {	/* no player on player killing */
		sprintf(OutputMessage,"Can't attack %s because player versus player combat is not allowed\r\n",usernext->name);
		send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);
		return(-1);
	}

	while(usernext->staminapoints > 0 && currentuser->staminapoints > 0) {

		/* user one attacks user two */
		HitPoints=rand() % (racenext->strength + 1) - 0;		/* random damage */
	
		if(usernext->status >= WIZARD) {
			sprintf(OutputMessage,"%s tries to attack %s but it just bounces off\r\n",usernext->next,currentuser->name);

			SendMessageToAllInRoom(currentuser->room,OutputMessage);
			continue;
		}
		else
		{
			sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",currentuser->name,usernext->name,HitPoints);
			SendMessageToAllInRoom(usernext->room,OutputMessage);

			usernext->staminapoints -= HitPoints;
	
 			currentuser->staminapoints += HitPoints;
			currentuser->experiencepoints += (HitPoints*currentuser->status);

			UpdateUser(currentuser,usernext->name,"",0,0,"",0,usernext->staminapoints,0,0,"","",0);
			UpdateUser(currentuser,usernext->name,"",0,0,"",0,currentuser->staminapoints,currentuser->experiencepoints,0,"","",0);
		}

		/* user two attacks user one */

		if(currentuser->status >= WIZARD) {
			sprintf(OutputMessage,"%s tries to attack %s but it just bounces off\r\n",currentuser->name,usernext->name);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);
		}
		else
		{
			HitPoints=rand() % (racenext->strength + 1) - 0;		/* random damage */

			sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",usernext->name,currentuser->name,HitPoints);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);

			currentuser->staminapoints -= HitPoints;
			currentuser->experiencepoints += (HitPoints*currentuser->status);

			UpdateUser(currentuser,usernext->name,"",0,0,"",0,usernext->staminapoints,0,0,"","",0);
			UpdateUser(currentuser,usernext->name,"",0,0,"",0,currentuser->staminapoints,currentuser->experiencepoints,0,"","",0);

		}

	}
}
	
for(MonsterLoopCount=0;MonsterLoopCount != rooms[currentroom->room].monstercount;MonsterLoopCount++) {

	if(strcmp(rooms[currentroom->room].roommonsters[MonsterLoopCount].name,target) == 0) {	/* if monster matches */
		found=TRUE;

		SaveStamina=rooms[currentroom->room].roommonsters[MonsterLoopCount].stamina;

		while(rooms[currentroom->room].roommonsters[MonsterLoopCount].stamina > 0 && currentuser->staminapoints > 0) {
			/* player attacks monster */

			HitPoints=rand() % (racenext->strength + 1) - 0;		/* random damage */
	
			sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",currentuser->name,rooms[currentroom->room].roommonsters[MonsterLoopCount].name,HitPoints);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);

			rooms[currentroom->room].roommonsters[MonsterLoopCount].stamina -= HitPoints;

			if(rooms[currentroom->room].roommonsters[MonsterLoopCount].stamina <= 0) { /* monster defeated */
				sprintf(OutputMessage,"%s has killed the %s!\r\n",currentuser->name,rooms[currentroom->room].roommonsters[MonsterLoopCount].name);
				SendMessageToAllInRoom(currentuser->room,OutputMessage);

				sprintf(OutputMessage,"You have gained %d stamina points\r\n",(SaveStamina*currentuser->status));
				send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);

				currentuser->staminapoints += SaveStamina;

				sprintf(OutputMessage,"You have gained %d experience points\r\n",(SaveStamina*currentuser->status));
				send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);

				currentuser->experiencepoints += (SaveStamina*currentuser->status);

				DeleteMonster(currentuser->room,MonsterLoopCount);
				return(0);
			}

			/* monster attacks user */
			HitPoints=rand() % (rooms[currentroom->room].roommonsters[MonsterLoopCount].damage + 1) - 0;	/* random damage */

			currentuser->staminapoints -= HitPoints;

			UpdateUser(currentuser,currentuser->name,"",0,0,"",0,currentuser->staminapoints,0,0,"","",0);

			sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",rooms[currentroom->room].roommonsters[MonsterLoopCount].name,currentuser->name,HitPoints);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);
		}

		break;
	}
}

if(found == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER);		/* missing player */
	return(-1);
}

return(0);
}
