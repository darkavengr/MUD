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
if((currentroom->attributes & ROOM_HAVEN) && currentuser->userlevel < WIZARD) {
	SetLastError(currentuser,ATTACK_HAVEN);
	return(-1);
}

/* find user */
UserPtr=GetUserPointerByName(target);		/* find user */
if(UserPtr != NULL) found=TRUE;			/* found user */

if(found == TRUE) {
	if(config.AllowPlayerKilling == FALSE) {	/* no player on player killing */
		snprintf(OutputMessage,BUF_SIZE,"Can't attack %s because player versus player combat is not allowed\r\n",UserPtr->username);
		send(currentuser->socket,OutputMessage,strlen(OutputMessage),0);
		return(-1);
	}

	while(UserPtr->staminapoints > 0 && currentuser->staminapoints > 0) {

		/* user one attacks user two */
		HitPoints=rand() % (currentuser->race.strength + 1) - 0;		/* random damage */
	
		if(UserPtr->userlevel >= WIZARD) {
			snprintf(OutputMessage,BUF_SIZE,"%s tries to attack %s but it just bounces off\r\n",UserPtr->next,currentuser->username);

			SendMessageToAllInRoom(currentuser->room,OutputMessage);
			return(0);
		}
		else
		{
			snprintf(OutputMessage,BUF_SIZE,"%s attacks %s causing %d points of damage\r\n",currentuser->username,UserPtr->username,HitPoints);
			SendMessageToAllInRoom(UserPtr->room,OutputMessage);

			UserPtr->staminapoints -= HitPoints;
	
 			currentuser->staminapoints += HitPoints;
			currentuser->experiencepoints += (HitPoints*currentuser->userlevel);

			/* update target user */
			UpdateUser(UserPtr,UserPtr->username,UserPtr->username,UserPtr->password,UserPtr->homeroom,UserPtr->userlevel,UserPtr->description,UserPtr->magicpoints,UserPtr->staminapoints,UserPtr->experiencepoints,UserPtr->gender,&UserPtr->race,&UserPtr->userclass,UserPtr->flags);

			/* update source user */
			UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,currentuser->userlevel,currentuser->description,currentuser->magicpoints,currentuser->staminapoints,currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags);
		}

		/* user two attacks user one */

		if(currentuser->userlevel >= WIZARD) {
			snprintf(OutputMessage,BUF_SIZE,"%s tries to attack %s but it just bounces off\r\n",currentuser->username,UserPtr->username);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);
			return(0);
		}
		else
		{
			HitPoints=rand() % (currentuser->race.strength + 1) - 0;		/* random damage */

			snprintf(OutputMessage,BUF_SIZE,"%s attacks %s causing %d points of damage\r\n",UserPtr->username,currentuser->username,HitPoints);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);

			currentuser->staminapoints -= HitPoints;
			currentuser->experiencepoints += (HitPoints*currentuser->userlevel);

			UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,currentuser->userlevel,currentuser->description,currentuser->magicpoints,currentuser->staminapoints,currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags);
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

			HitPoints=rand() % (currentuser->race.strength + 1) - 0;		/* random damage */
	
			snprintf(OutputMessage,BUF_SIZE,"%s attacks %s causing %d points of damage\r\n",currentuser->username,MonsterPtr->name,HitPoints);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);

			MonsterPtr->stamina -= HitPoints;

			if(MonsterPtr->stamina <= 0) { /* monster defeated */
				snprintf(OutputMessage,BUF_SIZE,"%s has killed the %s!\r\n",currentuser->username,MonsterPtr->name);
				SendMessageToAllInRoom(currentuser->room,OutputMessage);

				snprintf(OutputMessage,BUF_SIZE,"You have gained %d stamina points\r\n",(SaveStamina*currentuser->userlevel));
				send(currentuser->socket,OutputMessage,strlen(OutputMessage),0);

				currentuser->staminapoints += SaveStamina;

				snprintf(OutputMessage,BUF_SIZE,"You have gained %d experience points\r\n",(SaveStamina*currentuser->userlevel));
				send(currentuser->socket,OutputMessage,strlen(OutputMessage),0);

				currentuser->experiencepoints += (SaveStamina*currentuser->userlevel);

				UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,currentuser->userlevel,currentuser->description,currentuser->magicpoints,currentuser->staminapoints,currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags);

				DeleteMonster(currentuser->room,MonsterPtr->id);
				return(0);
			}

			/* monster attacks user */
			HitPoints=rand() % (MonsterPtr->damage + 1) - 0;	/* random damage */

			currentuser->staminapoints -= HitPoints;

			UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,currentuser->userlevel,currentuser->description,currentuser->magicpoints,currentuser->staminapoints,currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags);

			snprintf(OutputMessage,BUF_SIZE,"%s attacks %s causing %d points of damage\r\n",MonsterPtr->name,currentuser->username,HitPoints);
			SendMessageToAllInRoom(currentuser->room,OutputMessage);
		}
 
		MonsterPtr=FindNextMonsterInRoom(MonsterPtr);	
	}
}

if(found == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(0);
}

