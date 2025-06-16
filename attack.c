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

int attack(user *currentuser,char *target) {
int found;
int HitPoints;
user *UserPtr;
room *currentroom;
char *OutputMessage[BUF_SIZE];
CONFIG config;
int SaveStamina;
monster *MonsterPtr;

currentroom=currentuser->roomptr;
currentuser->race=currentuser->race;

GetConfigurationInformation(&config);

/*
* can't attack in haven rooms or attack wizards
*/
if((currentroom->attr & ROOM_HAVEN) && currentuser->status < WIZARD) {
	SetLastError(currentuser,ATTACK_HAVEN);
	return(-1);
}

/* find user */
UserPtr=GetUserPointerByName(target);		/* find user */
if(UserPtr != NULL) found=TRUE;			/* found user */

if(found == TRUE) {
	if(config.allowplayerkilling == FALSE) {	/* no player on player killing */
		sprintf(OutputMessage,"Can't attack %s because player versus player combat is not allowed\r\n",UserPtr->name);
		send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);
		return(-1);
	}

	while(UserPtr->staminapoints > 0 && currentuser->staminapoints > 0) {

		/* user one attacks user two */
		HitPoints=rand() % (currentuser->race->strength + 1) - 0;		/* random damage */
	
		if(UserPtr->status >= WIZARD) {
			sprintf(OutputMessage,"%s tries to attack %s but it just bounces off\r\n",UserPtr->next,currentuser->name);

			SendMessageToAllInRoom(currentuser->room,OutputMessage);
			return(0);
		}
		else
		{
			sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",currentuser->name,UserPtr->name,HitPoints);
			SendMessageToAllInRoom(UserPtr->room,OutputMessage);

			UserPtr->staminapoints -= HitPoints;
	
 			currentuser->staminapoints += HitPoints;
			currentuser->experiencepoints += (HitPoints*currentuser->status);

			/* update target user */
			UpdateUser(UserPtr,UserPtr->name,UserPtr->password,UserPtr->homeroom,UserPtr->status,UserPtr->desc,UserPtr->magicpoints,UserPtr->staminapoints,UserPtr->experiencepoints,UserPtr->gender,UserPtr->race,UserPtr->userclass,UserPtr->flags);

			/* update source user */
			UpdateUser(currentuser,currentuser->name,currentuser->password,currentuser->homeroom,currentuser->status,currentuser->desc,currentuser->magicpoints,currentuser->staminapoints,currentuser->experiencepoints,currentuser->gender,currentuser->race,currentuser->userclass,currentuser->flags);
		}

		/* user two attacks user one */

		if(currentuser->status >= WIZARD) {
			sprintf(OutputMessage,"%s tries to attack %s but it just bounces off\r\n",currentuser->name,UserPtr->name);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);
			return(0);
		}
		else
		{
			HitPoints=rand() % (currentuser->race->strength + 1) - 0;		/* random damage */

			sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",UserPtr->name,currentuser->name,HitPoints);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);

			currentuser->staminapoints -= HitPoints;
			currentuser->experiencepoints += (HitPoints*currentuser->status);

			UpdateUser(currentuser,UserPtr->name,"",0,0,"",0,UserPtr->staminapoints,0,0,"","",0);
			UpdateUser(currentuser,UserPtr->name,"",0,0,"",0,currentuser->staminapoints,currentuser->experiencepoints,0,"","",0);

		}

	}
}
	
MonsterPtr=FindFirstMonsterInRoom(currentuser->room);

while(MonsterPtr != NULL) {

	if(regexp(MonsterPtr->name,target) == 0) {	/* if monster matches */
		found=TRUE;

		SaveStamina=MonsterPtr->stamina;

		while(MonsterPtr->stamina > 0 && currentuser->staminapoints > 0) {
			/* player attacks monster */

			HitPoints=rand() % (currentuser->race->strength + 1) - 0;		/* random damage */
	
			sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",currentuser->name,MonsterPtr->name,HitPoints);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);

			MonsterPtr->stamina -= HitPoints;

			if(MonsterPtr->stamina <= 0) { /* monster defeated */
				sprintf(OutputMessage,"%s has killed the %s!\r\n",currentuser->name,MonsterPtr->name);
				SendMessageToAllInRoom(currentuser->room,OutputMessage);

				sprintf(OutputMessage,"You have gained %d stamina points\r\n",(SaveStamina*currentuser->status));
				send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);

				currentuser->staminapoints += SaveStamina;

				sprintf(OutputMessage,"You have gained %d experience points\r\n",(SaveStamina*currentuser->status));
				send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);

				currentuser->experiencepoints += (SaveStamina*currentuser->status);

				DeleteMonster(currentuser->room,MonsterPtr->id);
				return(0);
			}

			/* monster attacks user */
			HitPoints=rand() % (MonsterPtr->damage + 1) - 0;	/* random damage */

			currentuser->staminapoints -= HitPoints;

			UpdateUser(currentuser,currentuser->name,"",0,0,"",0,currentuser->staminapoints,0,0,"","",0);

			sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",MonsterPtr->name,currentuser->name,HitPoints);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);
		}
 
		MonsterPtr=FindNextMonsterInRoom(MonsterPtr);	
	}
}

if(found == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER);		/* missing player */
	return(-1);
}

return(0);
}
