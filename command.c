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

/* command interpreter */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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

#include "version.h"
#include "bool.h"
#include "directions.h"
#include "class.h"
#include "race.h"
#include "errors.h"
#include "room.h"
#include "command.h"
#include "config.h"
#include "user.h"
#include "string.h"
#include "world.h"

struct {
user *user;
char *statement;
unsigned int (*call_command)(user *,int,void *);		/* function pointer */
} statements[] = { {  NULL,"NORTH",&north_command },\
		 {  NULL,"N",&north_command },\
		 {  NULL,"NORTHWEST",&northwest_command },\
		 {  NULL,"NW",&northwest_command },\
		 {  NULL,"SOUTH",&south_command },\
		 {  NULL,"S",&south_command },\
		 {  NULL,"SOUTHEAST",&southeast_command },\
		 {  NULL,"SE",&southeast_command },\
		 {  NULL,"EAST",&east_command },\
		 {  NULL,"E",&east_command },\
		 {  NULL,"WEST",&west_command },\
		 {  NULL,"W",&west_command },\
		 {  NULL,"SOUTHWEST",&southwest_command },\
		 {  NULL,"SW",&southwest_command },\
		 {  NULL,"UP",&up_command },\
		 {  NULL,"U",&up_command },\
		 {  NULL,"DOWN",&down_command },\
		 {  NULL,"D",&down_command },\
		 {  NULL,"LOOK",&look_command },\
		 {  NULL,"WHO",&who_command },\
		 {  NULL,"SAY",&say_command },\
		 {  NULL,"WHISPER",&whisper_command },\
		 {  NULL,":",&pose_command },\
		 {  NULL,"POSE",&pose_command },\
		 {  NULL,"HOME",&home_command },\
		 {  NULL,"QUIT",&quit_command },\
		 {  NULL,"VERSION",&version_command },\
		 {  NULL,"DESCRIBE",&describe_command },\
		 {  NULL,"GET",&get_command },\
		 {  NULL,"DROP",&drop_command },\
		 {  NULL,"HELP",&help_command },\
		 {  NULL,"PASSWORD",&password_command },\
		 {  NULL,"CAST",&cast_command },\
		 {  NULL,"F",&fight_command },\
		 {  NULL,"SCORE",&score_command },\
		 {  NULL,"INV",&inv_command },\
		 {  NULL,"GIVE",give_command },\
		 {  NULL,"XYZZY",&xyzzy_command },\
		 {  NULL,"SETRACE",&setrace_command },\
		 {  NULL,"SET",&set_command },\
		 {  NULL,"SETHOME",&sethome_command },\
		 {  NULL,"SETGENDER",&setgender_command },\
		 {  NULL,"SETLEVEL",&setlevel_command },\
		 {  NULL,"SETCLASS",&setclass_command },\
		 {  NULL,"SETXP",&setxp_command },\
		 {  NULL,"SETMP",&setmp_command },\
		 {  NULL,"SETSP",&setsp_command },\
		 {  NULL,"BAN",&ban_command },\
		 {  NULL,"UNBAN",&unban_command },\
		 {  NULL,"KILL",&kill_command },\
		 {  NULL,"CREATE",&create_command },\
		 {  NULL,"DELETE",&delete_command },\
		 {  NULL,"RENAME",&rename_command },\
		 {  NULL,"CHOWN",&chown_command },\
		 {  NULL,"CHMOD",&chmod_command },\
		 {  NULL,"COPY",&copy_command },\
		 {  NULL,"MOVE",&move_command },\
		 {  NULL,"DIG",&dig_command },\
		 {  NULL,"FORCE",&force_command },\
		 {  NULL,"LISTBANS",&listban_command },\
		 {  NULL,"GO",&go_command },\
		 {  NULL,"WALL",&wall_command },\
		 {  NULL,"TAKE",&take_command },\
		 {  NULL,"RELOAD",&reload_command },\
		 {  NULL,"SHUTDOWN",&shutdown_command },\
		 {  NULL,"ADDCLASS",&addclass_command },\
		 {  NULL,"ADDRACE",&addrace_command },\
		 {  NULL,"DROPDEAD",&dropdead_command },\
		 {  NULL,"VISIBLE",&visible_command },\
		 {  NULL,"INVISIBLE",&invisible_command },\
		 {  NULL,"GAG",&gag_command },\
		 {  NULL,"UNGAG",&ungag_command },\
		 {  NULL,"SETEXIT",&setexit_command },\
		 {  NULL,"KICK",&kick_command },\
	         { NULL,NULL } };

char *NothingHappens="Nothing happens\r\n";
char *DirectionsMessage[]={ "North ","South ","East ","West ","Northeast ","Northwest ","Southeast ","Southwest ","Up ","Down " };
char *ExitsMessage="\r\nExits: ";
char *AllParameters;
char *AllParametersNotFirstTwo;
char *InThisRoomMessage="\r\nIn this room there is: ";
char *YouAreCarryingNothing="You are carrying nothing\r\n";

int ExecuteCommand(user *currentuser,char *command) {
char *CommandTokens[BUF_SIZE][BUF_SIZE];
int TokenCount;
int RoomLoop;
int StatementCount;
char *OutputMessage[BUF_SIZE];
char *uppercmd[BUF_SIZE];

if(!*command) {			/* no command */
	SetLastError(currentuser,NO_ERROR);
	return(0);
}

AllParameters=strpbrk(command," ");		/* point to all parameters */
if(AllParameters != NULL) AllParametersNotFirstTwo=strpbrk(AllParameters+1," ");

memset(CommandTokens,0,10*BUF_SIZE);
TokenCount=TokenizeLine(command,CommandTokens," ");			/* tokenize line */

StatementCount=0;

/* do statement */

do {
	if(statements[StatementCount].statement == NULL) break;

	/* if statement found, call it */

	ToUppercase(CommandTokens[0],uppercmd);

	if(strncmp(statements[StatementCount].statement,uppercmd,BUF_SIZE) == 0) return(statements[StatementCount].call_command(currentuser,TokenCount,CommandTokens));

	StatementCount++;

} while(statements[StatementCount].statement != NULL);

if(TokenCount > 1) {		/* do possible object verb action */
	if(DoObjectVerbAction(currentuser,CommandTokens[0],CommandTokens[1]) == -1) {

		if(GetLastError(currentuser) == VERB_NOT_FOUND) {	/* can't do that to an object */
			snprintf(OutputMessage,BUF_SIZE,"You can't %s %s\n",CommandTokens[0],CommandTokens[1]);

			send(currentuser->socket,OutputMessage,strlen(OutputMessage),0);
			return(0);		/* don't invoke error handler in main() */
		}
	}
	else
	{
		SetLastError(currentuser,NO_ERROR);
		return(0);
	}
}

SetLastError(currentuser,INVALID_COMMAND);
return(-1);
}

int north_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->roomptr->exits[NORTH]));
}

int south_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->roomptr->exits[SOUTH]));
}

int east_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->roomptr->exits[EAST]));
}

int west_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->roomptr->exits[NORTH]));
}

int northwest_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->roomptr->exits[NORTHWEST]));
}

int southwest_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->roomptr->exits[SOUTHWEST]));
}

int southeast_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->roomptr->exits[SOUTHEAST]));
}

int northeast_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->roomptr->exits[NORTHEAST]));
}

int up_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->roomptr->exits[UP]));
}

int down_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->roomptr->exits[DOWN]));
}

int LookAtRoom(user *currentuser,room *roomptr) {
char *RoomMessage[BUF_SIZE];
roomobject *ObjectNext;
user *UserPtr;
int found=FALSE;
int RoomExitCount;
monster *RoomMonster;

send(currentuser->socket,roomptr->name,strlen(roomptr->name),0);

if(currentuser->userlevel >= WIZARD) {		/* if wizard or higher, show room ID number */
	snprintf(RoomMessage,BUF_SIZE," (#%x)",roomptr->id);
	send(currentuser->socket,RoomMessage,strlen(RoomMessage),0);
}

send(currentuser->socket,"\r\n",2,0);
send(currentuser->socket,roomptr->description,strlen(roomptr->description),0);  

/* display exits */
send(currentuser->socket,ExitsMessage,strlen(ExitsMessage),0);

for(RoomExitCount=0;RoomExitCount < 11;RoomExitCount++) {
	if(currentuser->roomptr->exits[RoomExitCount] != 0) send(currentuser->socket,DirectionsMessage[RoomExitCount],strlen(DirectionsMessage[RoomExitCount]),0);
}

send(currentuser->socket,"\r\n",2,0);

/* display objects */

if(roomptr->roomobjects != NULL) {		/* display objects */
	send(currentuser->socket,InThisRoomMessage,strlen(InThisRoomMessage),0);

	ObjectNext=roomptr->roomobjects;

	while(ObjectNext != NULL) {
		send(currentuser->socket,ObjectNext->name,strlen(ObjectNext->name),0);

		if(currentuser->userlevel >= WIZARD) {		/* if wizard or higher, show object number */
			snprintf(RoomMessage,BUF_SIZE," (#%x)",ObjectNext->id);
			send(currentuser->socket,RoomMessage,strlen(RoomMessage),0);
		}
			
		if(ObjectNext->next != NULL) send(currentuser->socket,", ",2,0);

		ObjectNext=ObjectNext->next;
	}

	send(currentuser->socket,"\r\n",2,0);
}

/*
* display monsters in room
*
*/

send(currentuser->socket,"\r\n",2,0);

RoomMonster=FindFirstMonsterInRoom(currentuser->room);

while(RoomMonster != NULL) {
	if(currentuser->userlevel >= WIZARD) {		/* if wizard or higher, show object number */
		snprintf(RoomMessage,BUF_SIZE,"A %s (#%x) is here\r\n",RoomMonster->name,RoomMonster->id);
	}
	else
	{
		snprintf(RoomMessage,BUF_SIZE,"A %s is here\r\n",RoomMonster->name);
	}


	send(currentuser->socket,RoomMessage,strlen(RoomMessage),0);

	RoomMonster=FindNextMonsterInRoom(RoomMonster);
} 

/* display users in room */

UserPtr=FindFirstUser();		/* find first user */

while(UserPtr != NULL) {
	if((UserPtr->room == roomptr->id) && ((UserPtr->flags & USER_INVISIBLE) == 0)) {
		if(UserPtr->gender == MALE) {
			snprintf(RoomMessage,BUF_SIZE,"%s the %s is here\r\n",UserPtr->username,GetPointerToMaleTitles(UserPtr->userlevel));
		}
		else
		{
			snprintf(RoomMessage,BUF_SIZE,"%s the %s is here\r\n",UserPtr->username,GetPointerToFemaleTitles(UserPtr->userlevel));
		}

		send(currentuser->socket,RoomMessage,strlen(RoomMessage),0);
	}

	UserPtr=FindNextUser(UserPtr);		/* find next user */
}

return(0);
}

int look_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *temp[BUF_SIZE];
roomobject *ObjectNext;
user *UserPtr;
int found=FALSE;
monster *RoomMonster;
int IDNumber=0;
room *roomptr;

if(TokenCount < 2) {			/* look at current room */
	LookAtRoom(currentuser,currentuser->roomptr);
	return(0);
}

if((char) *CommandTokens[1] == '#') {			/* looking at room, object or monster by ID number */
	if(currentuser->userlevel < WIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	sscanf(CommandTokens[1],"#%x",&IDNumber);	/* get ID number */

	/* looking at room */
	roomptr=GetRoomPointer(IDNumber);

	if(roomptr != NULL) {
		LookAtRoom(currentuser,roomptr);
		return(0);
	}

	/* looking at object */

	ObjectNext=currentuser->roomptr->roomobjects;

	while(ObjectNext != NULL) {
		if(ObjectNext->id == IDNumber) {
			snprintf(temp,BUF_SIZE,"%s\r\n",ObjectNext->description);		/* show description */
			send(currentuser->socket,temp,strlen(temp),0);

			return(0);
		}

		ObjectNext=ObjectNext->next;
	}
	/* looking at monster */

	RoomMonster=FindFirstMonsterInRoom(currentuser->room);

	while(RoomMonster != NULL) {
		if(RoomMonster->id == IDNumber) {
			snprintf(temp,BUF_SIZE,"%s\r\n",RoomMonster->description);		/* show description */
			send(currentuser->socket,temp,strlen(temp),0);

			return(0);
		}

		RoomMonster=FindNextMonsterInRoom(RoomMonster);
	}
}

/* possibly looking at user, object or monster by name */ 

/* looking at object */

ObjectNext=currentuser->roomptr->roomobjects;

while(ObjectNext != NULL) {
	if(ObjectNext == NULL) break;

	if(regexp(ObjectNext->name,CommandTokens[1]) == TRUE) {
		snprintf(temp,BUF_SIZE,"%s\r\n",ObjectNext->description);		/* show description */
		send(currentuser->socket,temp,strlen(temp),0);

		found=TRUE;
	}

	ObjectNext=ObjectNext->next;
}
	
/* looking at user */

UserPtr=FindFirstUser();

while(UserPtr != NULL) {
	if((regexp(UserPtr->username,CommandTokens[1]) == TRUE) && (UserPtr->room == currentuser->room) && ((UserPtr->flags & USER_INVISIBLE) == 0)) {
		snprintf(temp,BUF_SIZE,"%s\r\n",UserPtr->description);		/* show description */
		send(currentuser->socket,temp,strlen(temp),0);

		found=TRUE;

		if(UserPtr->userlevel >= WIZARD) {		/* if the user is a wizard tell them they have looked at them */
			snprintf(temp,BUF_SIZE,"%s has looked at you\r\n",currentuser->username);
			send(UserPtr->socket,temp,strlen(temp),0);
		}
	}

	UserPtr=FindNextUser(UserPtr);		/* find next user */
}

/* looking at monster */

RoomMonster=FindFirstMonsterInRoom(currentuser->room);

while(RoomMonster != NULL) {
	if(regexp(CommandTokens[1],RoomMonster->name) == TRUE) {
		snprintf(temp,BUF_SIZE,"%s\r\n",RoomMonster->description);
		send(currentuser->socket,temp,strlen(temp),0);

		found=TRUE;
	}

	RoomMonster=FindNextMonsterInRoom(RoomMonster);
}

/* can't find it, so output error message and exit */

if(found == FALSE) {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  
	return(-1);
}

return(0);
}


int who_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *OutputMessage[BUF_SIZE];
char *NameBuffer[BUF_SIZE];
int found=FALSE;
user *UserPtr;

if(TokenCount <= 1) {
	strncpy(NameBuffer,"*",BUF_SIZE);          /* all users if no username */
}
else
{
	strncpy(NameBuffer,CommandTokens[1],BUF_SIZE);
}

UserPtr=FindFirstUser();		/* find first user */

while(UserPtr != NULL) {
	if((regexp(UserPtr->username,NameBuffer) == TRUE) && ((UserPtr->flags & USER_INVISIBLE) == 0)) {			/* found user */
		found=TRUE;

		if(UserPtr->gender == MALE) {
			snprintf(OutputMessage,BUF_SIZE,"%s the %s is in %s (#%x)\r\n",UserPtr->username,GetPointerToMaleTitles(UserPtr->userlevel),UserPtr->roomname,UserPtr->room);
		}
		else
		{
			snprintf(OutputMessage,BUF_SIZE,"%s the %s is in %s (#%x)\r\n",UserPtr->username,GetPointerToMaleTitles(UserPtr->userlevel),UserPtr->roomname,UserPtr->room);
		}

		send(currentuser->socket,OutputMessage,strlen(OutputMessage),0);
		found=TRUE;  
	}

	UserPtr=FindNextUser(UserPtr);		/* find next user */
}

if(found == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER);		/* unknown user */
	return(-1);
}

SetLastError(currentuser,NO_ERROR);
return(0);
}

int say_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *OutputMessage[BUF_SIZE];	

if((TokenCount < 2)) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if((currentuser->flags & USER_INVISIBLE) == 0) {
	snprintf(OutputMessage,BUF_SIZE,"%s Says, \"%s\"\r\n",currentuser->username,AllParameters);
}
else
{
	snprintf(OutputMessage,BUF_SIZE,"Somebody Says, \"%s\"\r\n",AllParameters);
}
	
return(SendMessageToAllInRoom(currentuser->room,OutputMessage));
}

int whisper_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SendMessage(currentuser,CommandTokens[1],AllParameters));
}

int pose_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *param[BUF_SIZE];
int count;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

pose(currentuser,AllParameters);
return(0);
}

int home_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->homeroom));
}

int quit_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
quit(currentuser);
}

int version_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *OutputMessage[BUF_SIZE];

snprintf(OutputMessage,BUF_SIZE,"%s %d.%d\r\n",MUD_NAME,MAJOR_VERSION,MINOR_VERSION);
send(currentuser->socket,OutputMessage,strlen(OutputMessage),0);
return(0);
}

int describe_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int ObjectID;
user *UserPtr;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if((char) *CommandTokens[1] == '#') {		/* setting object or room description */
	sscanf(CommandTokens[1],"#%x",&ObjectID);	/* get object ID */

	if(SetObjectDescription(currentuser,ObjectID,AllParametersNotFirstTwo) == -1) {		/* set object description */
		if(SetRoomDescription(currentuser,ObjectID,AllParametersNotFirstTwo) == -1) return(-1);	/* set room description */
	}

	SetLastError(currentuser,OBJECT_NOT_FOUND);
	return(-1);
}

/* if setting description for self */
if(strncmp(CommandTokens[1],"me",BUF_SIZE) == 0) {
	return(UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,currentuser->userlevel,AllParametersNotFirstTwo,currentuser->magicpoints,currentuser->staminapoints,currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags));
}
else if(strncmp(CommandTokens[1],"here",BUF_SIZE) == 0) {
	return(SetRoomDescription(currentuser,currentuser->roomptr->id,AllParametersNotFirstTwo));    /* if setting description for room */
}

/* set description for other user */

if(currentuser->userlevel < WIZARD) {
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

UserPtr=GetUserPointerByName(CommandTokens[1]);		/* get user information */
if(UserPtr == NULL) {			/* not found */
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(UpdateUser(UserPtr,UserPtr->username,UserPtr->username,UserPtr->password,UserPtr->homeroom,UserPtr->userlevel,AllParametersNotFirstTwo,UserPtr->magicpoints,UserPtr->staminapoints,UserPtr->experiencepoints,UserPtr->gender,&UserPtr->race,&UserPtr->userclass,UserPtr->flags));
}

int get_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);  
	return(-1);
}

return(PickUpObject(currentuser,CommandTokens[1]));
}

int drop_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(DropObject(currentuser,CommandTokens[1]));
}

int help_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(ShowHelp(currentuser,CommandTokens[1]));
}

int password_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(ChangePassword(currentuser,CommandTokens[1]));
}

int cast_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *uppercmd[BUF_SIZE];

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

ToUppercase(CommandTokens[2],uppercmd);

if(strncmp(uppercmd,"TO",BUF_SIZE) != 0) {		/* missing to */
	SetLastError(currentuser,SYNTAX_ERROR);
	return(-1);
}
	
return(CastSpell(currentuser,CommandTokens[1],CommandTokens[2]));
}

int fight_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(attack(currentuser,CommandTokens[1]));
}

int score_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
user *UserPtr;
char *OutputMessage[BUF_SIZE];
char *name[BUF_SIZE];
int found;
void *titleptr;

found=FALSE;

if(TokenCount <= 1) {			/* find score for current user */
	strncpy(name,currentuser->username,BUF_SIZE);
}
else
{
	if(currentuser->userlevel < WIZARD) {		/* not yet */
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	strncpy(name,CommandTokens[1],BUF_SIZE);
}

if(currentuser->gender == MALE) {		/* which user title */
	titleptr=GetPointerToMaleTitles(currentuser->userlevel);
}
else
{
	titleptr=GetPointerToFemaleTitles(currentuser->userlevel);
}

UserPtr=FindFirstUser();

do {
	if(regexp(UserPtr->username,name) == TRUE) {		/* found user */

		snprintf(OutputMessage,BUF_SIZE,"Magic Points:%d\r\nStamina Points:%d\r\nExperience Points:%d\r\nLevel: %s (%d)\r\n", \
										UserPtr->magicpoints,\
										UserPtr->staminapoints,\
										UserPtr->experiencepoints,\
										titleptr,\
										UserPtr->userlevel);

		send(currentuser->socket,OutputMessage,strlen(OutputMessage),0);
		found=TRUE;
	}

	UserPtr=FindNextUser(UserPtr);

} while(UserPtr != NULL);

if(found == FALSE)  {
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

SetLastError(currentuser,NO_ERROR);
return(0);
}

int inv_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *whichuser[BUF_SIZE];
char *OutputMessage[BUF_SIZE];
user *UserPtr;
roomobject *RoomObjectPtr;
int found=FALSE;

if(TokenCount <= 1) {
	strncpy(whichuser,currentuser->username,BUF_SIZE);   /* use default user */

}
else
{
	if(currentuser->userlevel < WIZARD) {		/* can't do this yet */
		SetLastError(currentuser,ACCESS_DENIED);
		return(0);
	}

	strncpy(whichuser,CommandTokens[1],BUF_SIZE);
}

UserPtr=FindFirstUser();

do {
	if(regexp(UserPtr->username,whichuser) == TRUE) {	/* found user */

		found=TRUE;

		if(UserPtr->carryobjects == NULL) {                   /* not carrying anything */
			if(UserPtr == currentuser) {
				send(currentuser->socket,YouAreCarryingNothing,strlen(YouAreCarryingNothing),0);
			}
			else
			{				
				snprintf(OutputMessage,BUF_SIZE,"%s is carrying nothing\r\n",UserPtr->username);
				send(currentuser->socket,OutputMessage,strlen(OutputMessage),0);
			}
		}
		else
		{
			snprintf(OutputMessage,BUF_SIZE,"%s is carrying: ",UserPtr->username);
			send(currentuser->socket,OutputMessage,strlen(OutputMessage),0);

			RoomObjectPtr=UserPtr->carryobjects;

			while(RoomObjectPtr != NULL) {
				send(currentuser->socket,RoomObjectPtr->name,strlen(RoomObjectPtr->name),0);
				send(currentuser->socket," ",1,0);
	
				RoomObjectPtr=RoomObjectPtr->next;
			}
	
			send(currentuser->socket,"\r\n",2,0);
		}
	}

	UserPtr=FindNextUser(UserPtr);

} while(UserPtr != NULL);


if(found == FALSE) {		/* user was not found */
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

SetLastError(currentuser,NO_ERROR);
return(0);
}

int give_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(GiveObjectToUser(currentuser,CommandTokens[1],CommandTokens[2]));
}

int xyzzy_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
send(currentuser->socket,NothingHappens,strlen(NothingHappens),0);
return(0);
}

/* ********************************
*        Wizard commands       *
********************************
*/

int setrace_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
user *UserPtr;

if(currentuser->userlevel < WIZARD) {		/* can't do this yet */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

UserPtr=GetUserPointerByName(CommandTokens[1]);		/* get user information */
if(UserPtr == NULL) {			/* not found */
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(UpdateUser(UserPtr,UserPtr->username,UserPtr->username,UserPtr->password,UserPtr->homeroom,UserPtr->userlevel,AllParametersNotFirstTwo,UserPtr->magicpoints,UserPtr->staminapoints,UserPtr->experiencepoints,UserPtr->gender,CommandTokens[2],&UserPtr->userclass,UserPtr->flags));
}

/*
* set configuration options */

int set_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *buf[BUF_SIZE];
CONFIG config;

GetConfigurationInformation(&config);

if(currentuser->userlevel < ARCHWIZARD) {		/* can't do this yet */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

if(strncmp(CommandTokens[1],"port",BUF_SIZE) == 0) {	
	config.port=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"server",BUF_SIZE) == 0) {	
	strncpy(config.server,CommandTokens[2],BUF_SIZE);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"object_generate_time",BUF_SIZE) == 0) {	
	config.ObjectGenerateTime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"config_save_time",BUF_SIZE) == 0) {
	config.ConfigurationSaveTime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"allow_player_killing",BUF_SIZE) == 0) {	
	if(strncmp(CommandTokens[2],"true",BUF_SIZE) == 0) config.AllowPlayerKilling=TRUE;
	if(strncmp(CommandTokens[2],"false",BUF_SIZE) == 0) config.AllowPlayerKilling=FALSE;

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"allow_new_accounts",BUF_SIZE) == 0) {	
	if(strncmp(CommandTokens[2],"true",BUF_SIZE) == 0) config.AllowNewAccounts=TRUE;
	if(strncmp(CommandTokens[2],"false",BUF_SIZE) == 0) config.AllowNewAccounts=FALSE;

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"monster_reset_time",BUF_SIZE) == 0) {	
	config.MonsterGenerateTime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"points_for_warrior",BUF_SIZE) == 0) {	
	config.PointsForWarrior=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"points_for_hero",BUF_SIZE) == 0) {	
	config.PointsForHero=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"points_for_warrior",BUF_SIZE) == 0) {	
	config.PointsForWarrior=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"points_for_champion",BUF_SIZE) == 0) {	
	config.PointsForChampion=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"points_for_superhero",BUF_SIZE) == 0) {	
	config.PointsForSuperhero=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"points_for_enchanter",BUF_SIZE) == 0) {	
	config.PointsForEnchanter=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"points_for_sorceror",BUF_SIZE) == 0) {	
	config.PointsForSorceror=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"points_for_necromancer",BUF_SIZE) == 0) {	
	config.PointsForNecromancer=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"points_for_legend",BUF_SIZE) == 0) {	
	config.PointsForLegend=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strncmp(CommandTokens[1],"points_for_wizard",BUF_SIZE) == 0) {	
	config.PointsForWizard=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

snprintf(buf,BUF_SIZE,"Invalid option %s\r\n",CommandTokens[2]);
send(currentuser->socket,buf,strlen(buf),0);
return(-1);
}

int sethome_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
user *UserPtr;

if(TokenCount == 1) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(TokenCount >= 2) {
	if(currentuser->userlevel < WIZARD) {			/* can't do this yet */
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	UserPtr=GetUserPointerByName(CommandTokens[1]);		/* get user information */
	if(UserPtr == NULL) {			/* not found */
		SetLastError(currentuser,UNKNOWN_USER);
		return(-1);
	}

	return(UpdateUser(UserPtr,UserPtr->username,UserPtr->username,UserPtr->password,atoi(CommandTokens[2]),UserPtr->userlevel,UserPtr->description,UserPtr->magicpoints,UserPtr->staminapoints,UserPtr->experiencepoints,UserPtr->gender,&UserPtr->race,&UserPtr->userclass,UserPtr->flags));
}

return(UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,atoi(CommandTokens[1]),currentuser->userlevel,currentuser->description,currentuser->magicpoints,currentuser->staminapoints,currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags));
}

int setgender_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
user *UserPtr;

if(TokenCount == 1) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(currentuser->userlevel < WIZARD) {			/* can't do this yet */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

UserPtr=GetUserPointerByName(CommandTokens[1]);		/* get user information */
if(UserPtr == NULL) {			/* not found */
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(UpdateUser(UserPtr,UserPtr->username,UserPtr->username,UserPtr->password,UserPtr->homeroom,UserPtr->userlevel,UserPtr->description,UserPtr->magicpoints,UserPtr->staminapoints,UserPtr->experiencepoints,atoi(CommandTokens[2]),&UserPtr->race,&UserPtr->userclass,UserPtr->flags));
}

int setlevel_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
user *UserPtr;

if(TokenCount == 1) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(TokenCount >= 2) {
	if(currentuser->userlevel < WIZARD) {			/* can't do this yet */
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	UserPtr=GetUserPointerByName(CommandTokens[1]);		/* get user information */
	if(UserPtr == NULL) {			/* not found */
		SetLastError(currentuser,UNKNOWN_USER);
		return(-1);
	}

	return(UpdateUser(UserPtr,CommandTokens[1],UserPtr->username,UserPtr->password,UserPtr->homeroom,atoi(CommandTokens[2]),UserPtr->description,UserPtr->magicpoints,UserPtr->staminapoints,UserPtr->experiencepoints,UserPtr->gender,&UserPtr->race,&UserPtr->userclass,UserPtr->flags));
}

return(UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,atoi(CommandTokens[2]),currentuser->description,currentuser->magicpoints,currentuser->staminapoints,currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags));
}

int setclass_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
user *UserPtr;

if(TokenCount == 1) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(currentuser->userlevel < WIZARD) {			/* can't do this yet */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

UserPtr=GetUserPointerByName(CommandTokens[1]);		/* get user information */
if(UserPtr == NULL) {			/* not found */
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(UpdateUser(UserPtr,UserPtr->username,UserPtr->username,UserPtr->password,UserPtr->homeroom,UserPtr->userlevel,UserPtr->description,UserPtr->magicpoints,UserPtr->staminapoints,atoi(CommandTokens[2]),UserPtr->gender,&UserPtr->race,&UserPtr->userclass,UserPtr->flags));
}

int setxp_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
user *UserPtr;

if(TokenCount == 1) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(TokenCount >= 2) {
	if(currentuser->userlevel < WIZARD) {			/* can't do this yet */
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	UserPtr=GetUserPointerByName(CommandTokens[1]);		/* get user information */
	if(UserPtr == NULL) {			/* not found */
		SetLastError(currentuser,UNKNOWN_USER);
		return(-1);
	}

	return(UpdateUser(UserPtr,UserPtr->username,UserPtr->username,UserPtr->password,UserPtr->homeroom,UserPtr->userlevel,UserPtr->description,UserPtr->magicpoints,UserPtr->staminapoints,atoi(CommandTokens[2]),UserPtr->gender,&UserPtr->race,&UserPtr->userclass,UserPtr->flags));
}

return(UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,currentuser->userlevel,currentuser->description,currentuser->magicpoints,currentuser->staminapoints,atoi(CommandTokens[2]),currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags));
}

int setmp_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
user *UserPtr;

if(TokenCount == 1) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(TokenCount >= 2) {
	if(currentuser->userlevel < WIZARD) {			/* can't do this yet */
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	UserPtr=GetUserPointerByName(CommandTokens[1]);		/* get user information */
	if(UserPtr == NULL) {			/* not found */
		SetLastError(currentuser,UNKNOWN_USER);
		return(-1);
	}

	return(UpdateUser(UserPtr,UserPtr->username,UserPtr->username,UserPtr->password,UserPtr->homeroom,UserPtr->userlevel,UserPtr->description,atoi(CommandTokens[2]),UserPtr->staminapoints,UserPtr->experiencepoints,UserPtr->gender,&UserPtr->race,&UserPtr->userclass,UserPtr->flags));
}

return(UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,currentuser->userlevel,currentuser->description,atoi(CommandTokens[2]),currentuser->staminapoints,currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags));
}

int setsp_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
user *UserPtr;

if(TokenCount == 1) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(TokenCount >= 2) {
	if(currentuser->userlevel < WIZARD) {			/* can't do this yet */
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	UserPtr=GetUserPointerByName(CommandTokens[1]);		/* get user information */
	if(UserPtr == NULL) {			/* not found */
		SetLastError(currentuser,UNKNOWN_USER);
		return(-1);
	}

	return(UpdateUser(UserPtr,UserPtr->username,UserPtr->username,UserPtr->password,UserPtr->homeroom,UserPtr->userlevel,UserPtr->description,UserPtr->magicpoints,atoi(CommandTokens[2]),UserPtr->experiencepoints,UserPtr->gender,&UserPtr->race,&UserPtr->userclass,UserPtr->flags));
}

return(UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,currentuser->userlevel,currentuser->description,currentuser->magicpoints,atoi(CommandTokens[2]),currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags));
}


int BanUserByIPAddress_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(BanUserByIPAddress(currentuser,CommandTokens[1],CommandTokens[2]));
}

int unban_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(UnBanUserByIPAddress(currentuser,CommandTokens[1]));
}

int ban_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(BanUserByIPAddress(currentuser,CommandTokens[1],CommandTokens[2]));
}

int kill_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(KillUser(currentuser,CommandTokens[1]));
}

int create_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
roomobject newobject;
char *temp[BUF_SIZE];

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

memset(&newobject,0,sizeof(roomobject));

strncpy(&newobject.name,CommandTokens[1],BUF_SIZE);
strncpy(&newobject.owner,currentuser->username,BUF_SIZE);

snprintf(temp,BUF_SIZE,"Describe this object with desc %s",newobject.name);
strncpy(newobject.description,temp,BUF_SIZE);

return(CreateObject(currentuser,&newobject,currentuser->room));
}

int delete_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int ObjectID;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if((char) *CommandTokens[1] != '#') {				/* not a valid ID number */
	SetLastError(currentuser,SYNTAX_ERROR);
	return(-1);
}

sscanf(CommandTokens[1],"#%x",&ObjectID);

return(DeleteObject(currentuser,ObjectID));
}

int rename_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int ObjectID;
user *UserPtr;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(*CommandTokens[1] == '#') {				/* valid ID number */
	if(RenameObject(currentuser,ObjectID,CommandTokens[2]) == 0) return(-1);	/* rename object */

	return(0);
}

/* renaming user */

if(currentuser->userlevel < WIZARD) {			/* can't do this yet */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

UserPtr=GetUserPointerByName(CommandTokens[1]);		/* get user information */
if(UserPtr == NULL) {			/* not found */
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(UpdateUser(UserPtr,UserPtr->username,CommandTokens[2],UserPtr->password,UserPtr->homeroom,UserPtr->userlevel,UserPtr->description,UserPtr->magicpoints,UserPtr->staminapoints,UserPtr->experiencepoints,UserPtr->gender,&UserPtr->race,&UserPtr->userclass,UserPtr->flags));
}

int chown_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {		/* set object owner */
int ObjectID;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if((char) *CommandTokens[1] != '#') {				/* not a valid ID number */
	SetLastError(currentuser,SYNTAX_ERROR);
	return(-1);
}

sscanf(CommandTokens[1],"#%x",&ObjectID);

return(SetOwner(currentuser,CommandTokens[1],CommandTokens[2]));
}

int chmod_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int ObjectID;
char *AttributePtr;
int attributes=0;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if((char) *CommandTokens[1] != '#') {				/* not a valid ID number */
	SetLastError(currentuser,SYNTAX_ERROR);
	return(-1);
}

sscanf(CommandTokens[1],"#%x",&ObjectID);

AttributePtr=CommandTokens[2];

while(*AttributePtr != 0) {
	if(*AttributePtr == '+') {			/* setting attribute */
		AttributePtr++;

		switch(*AttributePtr++) {
			case 'd':
				attributes |= OBJECT_DELETE_OWNER;
				break;

			case 'D':
				attributes |= OBJECT_DELETE_PUBLIC;
				break;
	
			case 'm':
				attributes |= OBJECT_MOVEABLE_OWNER;
				break;

			case 'M':
				attributes |= OBJECT_MOVEABLE_PUBLIC;
				break;

			case 'p':
				attributes |= OBJECT_PICKUP_OWNER;
				break;

			case 'P':
				attributes |= OBJECT_PICKUP_PUBLIC;
				break;

			case 'r':
				attributes |= OBJECT_RENAME_OWNER;
				break;

			case 'R':
				attributes |= OBJECT_RENAME_PUBLIC;
				break;

			case 't':
				attributes |= OBJECT_TEMPORARY;
				break;

			default:
				SetLastError(currentuser,SYNTAX_ERROR);
				return(-1);
			}
	}
	else if(*AttributePtr == '-') {
		
		AttributePtr++;

		switch(*AttributePtr++) {
			case 'd':
				attributes |= ~OBJECT_DELETE_OWNER;
				break;

			case 'D':
				attributes |= ~OBJECT_DELETE_PUBLIC;
				break;

			case 'm':
				attributes |= ~OBJECT_MOVEABLE_OWNER;
				break;	

			case 'M':
				attributes |= ~OBJECT_MOVEABLE_PUBLIC;
				break;

			case 'p':
				attributes |= ~OBJECT_PICKUP_OWNER;
				break;

			case 'P':
				attributes |= ~OBJECT_PICKUP_PUBLIC;
				break;

			case 'r':
				attributes |= ~OBJECT_RENAME_OWNER;
				break;

			case 'R':
				attributes |= ~OBJECT_RENAME_PUBLIC;
				break;

			case 't':
				attributes |= ~OBJECT_TEMPORARY;
				break;

			default:
				SetLastError(currentuser,SYNTAX_ERROR);
				return(-1);
			}
	}
	else
	{
		AttributePtr++;

		SetLastError(currentuser,SYNTAX_ERROR);
		return(-1);
	}
}

return(SetObjectAttributes(currentuser,ObjectID,attributes));
}

int copy_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(CopyObject(currentuser,CommandTokens[1],atoi(CommandTokens[2])));
}

int move_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
user *UserPtr;
int ObjectID;
int DestinationObjectID;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if((char) *CommandTokens[1] != '#') {				/* not a valid ID number */
	SetLastError(currentuser,SYNTAX_ERROR);
	return(-1);
}

sscanf(CommandTokens[1],"#%x",&ObjectID);

if(*CommandTokens[2] == '#') sscanf(CommandTokens[1],"#%x",&DestinationObjectID);	/* get destination ID */

if(CopyObject(currentuser,CommandTokens[1],ObjectID) == 0) return(DeleteObject(currentuser,ObjectID));

/* move player */

UserPtr=GetUserPointerByName(CommandTokens[1]);		/* find user */
if(UserPtr != NULL) {			/* found user */
	go(UserPtr,DestinationObjectID);

	SetLastError(currentuser,NO_ERROR);
	return(0);
}

/* copy room */

if((GetRoomPointer(DestinationObjectID) == NULL) || (GetRoomPointer(ObjectID) == NULL)) {	/* room not found */
	SetLastError(currentuser,OBJECT_NOT_FOUND);
	return(-1);
}

memcpy(GetRoomPointer(DestinationObjectID),GetRoomPointer(ObjectID),sizeof(room));

SetLastError(currentuser,NO_ERROR);
return(0);
}

int dig_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int CreateDirection=CreateRoom(currentuser,CommandTokens[1]);
char *CreateMessage[BUF_SIZE];

snprintf(CreateMessage,BUF_SIZE,"A room has been created to the %s\r\n",DirectionsMessage[CreateDirection]);
send(currentuser->socket,CreateMessage,strlen(CreateMessage),0);

SetLastError(currentuser,NO_ERROR);
return(0);
}

int force_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(ForceUser(currentuser,CommandTokens[1],AllParametersNotFirstTwo));
}

int listban_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(ListBans(currentuser,CommandTokens[1]));
}

int go_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(currentuser->userlevel < WIZARD) {		/* can't do that */
	SetLastError(currentuser,ACCESS_DENIED);  
	return(-1);
}

return(go(currentuser,atoi(CommandTokens[1])));
}

int wall_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(wall(currentuser,AllParameters));
}

int take_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(TakeObject(currentuser,CommandTokens[1],CommandTokens[2]));
}

int reload_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(currentuser->userlevel < ARCHWIZARD) {		/* can't do this yet */
	SetLastError(currentuser,ACCESS_DENIED);  
	return(-1);
}

return(GetConfiguration());
}

int shutdown_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(ShutdownServer(currentuser,CommandTokens[1]));
}

int addclass_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
userclass class;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

strncpy(class.name,CommandTokens[1],BUF_SIZE);
	
if(AddNewClass(currentuser,&class) == -1) {
	SetLastError(currentuser,NO_MEM);  
	return(-1);
}

return(0);
}

int addrace_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
race race;

if(TokenCount < 9) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

strncpy(race.name,CommandTokens[1],BUF_SIZE);
race.magic=atoi(CommandTokens[2]);
race.strength=atoi(CommandTokens[3]);
race.agility=atoi(CommandTokens[4]);
race.luck=atoi(CommandTokens[5]);
race.wisdom=atoi(CommandTokens[6]);
race.intelligence=atoi(CommandTokens[7]);
race.stamina=atoi(CommandTokens[8]);

if(AddNewRace(currentuser,&race) == -1) {
	SetLastError(currentuser,NO_MEM);
	return(-1);
}

return(0);
}

int dropdead_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(UpdateUser(currentuser,currentuser->username,currentuser->username,"",0,0,"",0,0,0,0,"","",0));
}

int invisible_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(SetVisibleMode(currentuser,CommandTokens[1],FALSE));
}

int visible_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(SetVisibleMode(currentuser,CommandTokens[1],TRUE));
}

int gag_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(GagUser(currentuser,CommandTokens[1],TRUE));
}

int ungag_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(GagUser(currentuser,CommandTokens[1],FALSE));
}

int setexit_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int room;
int target;

if(strncmp(CommandTokens[1],"here",BUF_SIZE) == 0) {
	room=currentuser->room;
}
else
{
	sscanf(CommandTokens[1],"#%x",&room);
}

if(strncmp(CommandTokens[3],"here",BUF_SIZE) == 0) {
	target=currentuser->room;
}
else
{
	sscanf(CommandTokens[1],"#%x",&target);
}


return(SetExit(currentuser,room,CommandTokens[2],target));
}

int kick_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *KickMessage[BUF_SIZE];
user *UserPtr;
bool UserFound=FALSE;

if(currentuser->userlevel < WIZARD) {		/* can't do that */
	SetLastError(currentuser,ACCESS_DENIED);  
	return(-1);
}

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}


/* display users in room */

UserPtr=FindFirstUser();		/* find first user */

while(UserPtr != NULL) {
	if(regexp(UserPtr->username,CommandTokens[1]) == TRUE) {		/* found user */
		/* send message */

		if(*CommandTokens[2]) {
			snprintf(KickMessage,BUF_SIZE,"\r\n** %s was kicked off by %s (%s)\r\n",UserPtr->username,currentuser->username,AllParametersNotFirstTwo);
		}
		else
		{
			snprintf(KickMessage,BUF_SIZE,"\r\n** %s was kicked off by %s\r\n",UserPtr->username,currentuser->username);
		}

		SendMessageToAllInRoom(UserPtr->room,KickMessage);

		DisconnectUser(currentuser,UserPtr->username);

		UserFound=TRUE;
	}

	UserPtr=FindNextUser(UserPtr);		/* find next user */
}


if(UserFound == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(0);
}

