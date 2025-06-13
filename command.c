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
		 {  NULL,"SPELL",&spell_command },\
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
		 {  NULL,"SETCLASSS",&setclass_command },\
		 {  NULL,"SETXP",&setxp_command },\
		 {  NULL,"SETMP",&setmp_command },\
		 {  NULL,"SETSP",&setsp_command },\
		 {  NULL,"BANIP",&ban_command },\
		 {  NULL,"UNBAN",&unban_command },\
		 {  NULL,"BAN",&ban_command },\
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
		 {  NULL,"LISTBAN",&listban_command },\
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
	         { NULL,NULL } };

char *NothingHappens="Nothing happens\r\n";
char *DirectionsMessage[]={ "North ","South ","East ","West ","Northeast ","Northwest ","Southeast ","Southwest ","Up ","Down " };
char *ExitsMessage="\r\nExits: ";
char *AllParameters;
char *AllParametersNotFirstTwo;

int ExecuteCommand(user *currentuser,char *command) {
char *CommandTokens[BUF_SIZE][BUF_SIZE];
int TokenCount;
int RoomLoop;
int StatementCount;

if(!*command) return(0);			/* no command */

AllParameters=strpbrk(command," ");		/* point to all parameters */
if(AllParameters != NULL) AllParametersNotFirstTwo=strpbrk(AllParameters+1," ");

memset(CommandTokens,0,10*BUF_SIZE);
TokenCount=TokenizeLine(command,CommandTokens," ");			/* tokenize line */

StatementCount=0;

/* do statement */

do {
	if(statements[StatementCount].statement == NULL) break;

	ToUppercase(CommandTokens[0]);

	/* if statement found, call it */

	if(strcmp(statements[StatementCount].statement,CommandTokens[0]) == 0) {
		return(statements[StatementCount].call_command(currentuser,TokenCount,CommandTokens));
	}

	StatementCount++;

} while(statements[StatementCount].statement != NULL);

SetLastError(currentuser,BAD_COMMAND);
return(-1);
}

int north_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[NORTH]));
}

int south_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[SOUTH]));
}

int east_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[EAST]));
}

int west_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[NORTH]));
}

int northwest_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[NORTHWEST]));
}

int southwest_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[SOUTHWEST]));
}

int southeast_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[SOUTHEAST]));
}

int northeast_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[NORTHEAST]));
}

int up_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[UP]));
}

int down_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[DOWN]));
}

int look_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *RoomMessage[BUF_SIZE];
roomobject *ObjectNext;
user *UserPtr;
room *CurrentRoom;
int found=FALSE;
int RoomExitCount;
monster *RoomMonster;

CurrentRoom=currentuser->roomptr;
	
/* no name, so look at current room */

if(TokenCount <= 1) {				/* display name */

	send(currentuser->handle,CurrentRoom->name,strlen(CurrentRoom->name),0);

	if(currentuser->status >= WIZARD) {		/* if wizard or higher, show object number */
		sprintf(RoomMessage," (#%x)",CurrentRoom->id);
		send(currentuser->handle,RoomMessage,strlen(RoomMessage),0);
	}

	send(currentuser->handle,"\r\n",2,0);
	send(currentuser->handle,CurrentRoom->desc,strlen(CurrentRoom->desc),0);  

	send(currentuser->handle,ExitsMessage,strlen(ExitsMessage),0);  		/* display exits */

	for(RoomExitCount=0;RoomExitCount<11;RoomExitCount++) {
		if(CurrentRoom->exits[RoomExitCount] != 0) send(currentuser->handle,DirectionsMessage[RoomExitCount],strlen(DirectionsMessage[RoomExitCount]),0);
	}

	send(currentuser->handle,"\r\n",2,0);

	if(CurrentRoom->roomobjects != NULL) {		/* display objects */
		send(currentuser->handle,"\r\n",2,0);
		send(currentuser->handle,RoomMessage,strlen(RoomMessage),0);
		ObjectNext=CurrentRoom->roomobjects;

		while(ObjectNext != NULL) {

			if(currentuser->status >= WIZARD) {		/* if wizard or higher, show object number */
				sprintf(RoomMessage," (#%x), ",ObjectNext->id);
				send(currentuser->handle,RoomMessage,strlen(RoomMessage),0);
			}
			else
			{
				send(currentuser->handle,ObjectNext->name,strlen(ObjectNext->name),0);
				send(currentuser->handle,", ",2,0);
			}

			ObjectNext=ObjectNext->next;
		}

		send(currentuser->handle,"\r\n",2,0);
	}

/*
* display monsters in room
*
*/

	send(currentuser->handle,"\r\n",2,0);

	RoomMonster=FindFirstMonsterInRoom(currentuser->room);

	while(RoomMonster != NULL) {

		if(currentuser->status >= WIZARD) {		/* if wizard or higher, show object number */
			sprintf(RoomMessage,"A %s (#%x) is here\r\n",RoomMonster->name,RoomMonster->id);
		}
		else
		{
			sprintf(RoomMessage,"A %s is here\r\n",RoomMonster->name);
		}

		send(currentuser->handle,RoomMessage,strlen(RoomMessage),0);

		RoomMonster=FindNextMonsterInRoom(RoomMonster);
	} 


	/* display users in room */

	UserPtr=FindFirstUser();		/* find first user */

	while(UserPtr != NULL) {
		if((UserPtr->loggedin == TRUE) && (UserPtr->room == currentuser->room)) {
			if(UserPtr->gender == MALE) {
				sprintf(RoomMessage,"%s the %s is here\r\n",UserPtr->name,GetPointerToMaleTitles(UserPtr->status));
			}
			else
			{
				sprintf(RoomMessage,"%s the %s is here\r\n",UserPtr->name,GetPointerToFemaleTitles(UserPtr->status));
			}

			send(currentuser->handle,RoomMessage,strlen(RoomMessage),0);
		}

		UserPtr=FindNextUser(UserPtr);		/* find next user */
	}

	return(0);
}

/* looking at object or person */

ObjectNext=CurrentRoom->roomobjects;

while(NULL != ObjectNext) {
	if(ObjectNext == NULL) break;

	if(regexp(ObjectNext->name,CommandTokens[1]) == TRUE) {
		send(currentuser->handle,ObjectNext->desc,strlen(ObjectNext->desc),0); /* if object matches */
		found=TRUE;
	}

	ObjectNext=ObjectNext->next;
}
	

/* if not not object or room, search for user */

UserPtr=FindFirstUser();		/* find first user */

while(UserPtr != NULL) {
	if((regexp(UserPtr->name,CommandTokens[1]) == TRUE) && (UserPtr->loggedin == TRUE) && (UserPtr->room == currentuser->room)) {
		sprintf(RoomMessage,"%s\r\n",UserPtr->desc);		/* show description */
		send(currentuser->handle,RoomMessage,strlen(RoomMessage),0);

		found=TRUE;

	/* if the user is a wizard tell them they have looked at them */

		if(UserPtr->status >= WIZARD) {
			sprintf(RoomMessage,"%s has looked at you\r\n",currentuser->name);
			send(UserPtr->handle,RoomMessage,strlen(RoomMessage),0);
		}
	}

	UserPtr=FindNextUser(UserPtr);		/* find next user */
}

/*
* if monster
*
*/

RoomMonster=FindFirstMonsterInRoom(currentuser->room);

while(RoomMonster != NULL) {
	if(regexp(CommandTokens[1],RoomMonster->name) == TRUE) {
		send(currentuser->handle,RoomMonster->desc,strlen(RoomMonster->desc),0);

		found=TRUE;
		return(0);
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
	strcpy(NameBuffer,"*");          /* all users if no username */
}
else
{
	strcpy(NameBuffer,CommandTokens[1]);          
}

UserPtr=FindFirstUser();		/* find first user */

while(UserPtr != NULL) {
	if((regexp(UserPtr->name,NameBuffer) == TRUE) && (UserPtr->loggedin == TRUE)  && ((UserPtr->flags & USER_INVISIBLE) == 0)) {			/* found user */
		found=TRUE;

		if(UserPtr->gender == MALE) {
			sprintf(OutputMessage,"%s the %s is in %s (#%d)\r\n",UserPtr->name,GetPointerToMaleTitles(UserPtr->status),UserPtr->roomname,UserPtr->room);
		}
		else
		{
			sprintf(OutputMessage,"%s the %s is in %s (#%d)\r\n",UserPtr->name,GetPointerToMaleTitles(UserPtr->status),UserPtr->roomname,UserPtr->room);
		}

		send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);
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
	sprintf(OutputMessage,"%s Says, \"%s\"\r\n",currentuser->name,AllParameters);
}
else
{
	sprintf(OutputMessage,"Somebody Says, \"%s\"\r\n",AllParameters);
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

sprintf(OutputMessage,"%s %d.%d\r\n",MUD_NAME,MAJOR_VERSION,MINOR_VERSION);
send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);
return(0);
}

int describe_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int ObjectID;
room *CurrentRoom=currentuser->roomptr;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if((char) *CommandTokens[1] == '#') {		/* setting object or  description */
	sscanf(CommandTokens[1],"#%d",&ObjectID);	/* get object ID */

	if(SetObjectDescription(currentuser,ObjectID,AllParametersNotFirstTwo) == -1) {	/* not object */
		return(SetRoomDescription(currentuser,ObjectID,AllParametersNotFirstTwo));
	}
}

/* if setting description for self */
if(strcmp(CommandTokens[1],"me") == 0) return(UpdateUser(currentuser,currentuser->name,"",0,0,AllParametersNotFirstTwo,0,currentuser->staminapoints,0,0,"","",0));

if(strcmp(CommandTokens[1],"here") == 0) return(SetRoomDescription(currentuser,CurrentRoom->id,AllParametersNotFirstTwo));    /* if setting description for room */

/* set description for other user */

return(UpdateUser(currentuser,currentuser->name,"",0,0,AllParametersNotFirstTwo,0,currentuser->staminapoints,0,0,"","",0));
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

int spell_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
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
	strcpy(name,currentuser->name);
}
else
{
	if(currentuser->status < WIZARD) {		/* not yet */
		SetLastError(currentuser,NOT_YET);
		return(-1);
	}

	strcpy(name,CommandTokens[1]);
}

if(currentuser->gender == MALE) {		/* which user title */
	titleptr=GetPointerToMaleTitles(currentuser->status);
}
else
{
	titleptr=GetPointerToFemaleTitles(currentuser->status);
}

UserPtr=FindFirstUser();

do {
	if(regexp(UserPtr->name,name) == TRUE) {		/* found user */

		sprintf(OutputMessage,"Magic Points:%d\r\nStamina Points:%d\r\nExperience Points:%d\r\nLevel: %s (%d)\r\n", \
										UserPtr->magicpoints,\
											UserPtr->staminapoints,\
										UserPtr->experiencepoints,\
										titleptr,\
										UserPtr->status);

		send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);
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
	strcpy(whichuser,currentuser->name);   /* use default user */

}
else
{
	if(currentuser->status < WIZARD) {		/* can't do this yet */
		SetLastError(currentuser,NOT_YET);
		return(0);
	}

	strcpy(whichuser,CommandTokens[1]);
}

UserPtr=FindFirstUser();

do {
	if((regexp(UserPtr->name,whichuser) == TRUE) && (UserPtr->loggedin == TRUE)) {	/* found user */

		found=TRUE;

		if(UserPtr->carryobjects == NULL) {                   /* not carrying anything */
			sprintf(OutputMessage,"%s is carrying nothing\r\n",UserPtr->name);

			send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);
			continue;
		}

		sprintf(OutputMessage,"%s is carrying: ",UserPtr->name);
		send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);

		RoomObjectPtr=UserPtr->carryobjects;

		while(RoomObjectPtr != NULL) {
			send(currentuser->handle,RoomObjectPtr->name,strlen(RoomObjectPtr->name),0);	/* display objects in inventory */
			send(currentuser->handle," ",1,0);
	
			RoomObjectPtr=RoomObjectPtr->next;
		}
	
		send(currentuser->handle,"\r\n",2,0);
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
send(currentuser->handle,NothingHappens,strlen(NothingHappens),0);
return(0);
}

/* ********************************
*        Wizard commands       *
********************************
*/

int setrace_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(currentuser->status < WIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

UpdateUser(currentuser,CommandTokens[1],"",0,0,"",0,0,0,0,CommandTokens[1],"",0);
return(0);
}

/*
* set configuration options */

int set_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *buf[BUF_SIZE];
CONFIG config;

GetConfigurationInformation(&config);

if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(strcmp(CommandTokens[1],"port") == 0) {	
	config.mudport=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"server") == 0) {	
	strcpy(config.mudserver,CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"object_reset_time") == 0) {	
	config.objectresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"database_save_time") == 0) {	
	config.databaseresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"user_reset_time") == 0) {	
	config.userresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"database_save_time") == 0) {	
	config.databaseresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"config_save_time") == 0) {	
	config.configsavetime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"allow_player_killing") == 0) {	
	if(strcmp(CommandTokens[2],"true") == 0) config.allowplayerkilling=TRUE;
	if(strcmp(CommandTokens[2],"false") == 0) config.allowplayerkilling=FALSE;

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"allow_new_accounts") == 0) {	
	if(strcmp(CommandTokens[2],"true") == 0) config.allownewaccounts=TRUE;
	if(strcmp(CommandTokens[2],"false") == 0) config.allownewaccounts=FALSE;

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"monster_reset_time") == 0) {	
	config.monsterresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"ban_reset_time") == 0) {	
	config.banresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"points_for_warrior") == 0) {	
	config.pointsforwarrior=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"points_for_hero") == 0) {	
	config.pointsforhero=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"points_for_warrior") == 0) {	
	config.pointsforwarrior=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"points_for_champion") == 0) {	
	config.pointsforchampion=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"points_for_superhero") == 0) {	
	config.pointsforsuperhero=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"points_for_enchanter") == 0) {	
	config.pointsforenchanter=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"points_for_sorceror") == 0) {	
	config.pointsforsorceror=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"points_for_necromancer") == 0) {	
	config.pointsfornecromancer=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"points_for_legend") == 0) {	
	config.pointsforlegend=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}
else if(strcmp(CommandTokens[1],"points_for_wizard") == 0) {	
	config.pointsforwizard=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

sprintf(buf,"Bad option %s\r\n",CommandTokens[2]);
send(currentuser->handle,buf,strlen(buf),0);
return(-1);
}

int sethome_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(TokenCount <= 1 && currentuser->status < WIZARD) {			/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

return(UpdateUser(currentuser,CommandTokens[0],"",atoi(CommandTokens[1]),0,"",0,0,0,0,"","",0));
}

int setgender_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetUserGender(currentuser,CommandTokens[1],CommandTokens[2]));
}

int setlevel_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetUserLevel(currentuser,CommandTokens[1],CommandTokens[2]));
}

int setclass_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(UpdateUser(currentuser,CommandTokens[1],"",0,0,"",0,0,0,0,"",CommandTokens[2],0));
}

int setxp_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetUserPoints(currentuser,CommandTokens[1],CommandTokens[2],EXPERIENCEPOINTS));
}

int setmp_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetUserPoints(currentuser,CommandTokens[1],CommandTokens[2],MAGICPOINTS));
}

int setsp_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetUserPoints(currentuser,CommandTokens[1],CommandTokens[2],STAMINAPOINTS));
}

int BanUserByIPAddress_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(BanUserByIPAddress(currentuser,CommandTokens[1]));
}

int unban_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(UnBanUserByIPAddress(currentuser,CommandTokens[1]));
}

int ban_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(BanUserByName(currentuser,CommandTokens[1]));
}

int kill_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(KillUser(currentuser,CommandTokens[1]));
}

int create_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(CreateObject(currentuser,CommandTokens[1]));
}

int delete_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int ObjectID;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(*CommandTokens[1] != '#') {				/* not a valid ID number */
	SetLastError(currentuser,SYNTAX_ERROR);
	return(-1);
}

sscanf(CommandTokens[1],"#%d",&ObjectID);

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
}
else
{
	UserPtr=GetUserPointerByName(CommandTokens[1]);

	if(UserPtr == NULL) {		/* user not found */
		SetLastError(currentuser,UNKNOWN_USER);
		return(-1);
	}

	strcpy(UserPtr->name,CommandTokens[2]);		/* set username */

	return(UpdateUser(currentuser,CommandTokens[0],"",0,0,"",0,0,0,0,"","",0));
}

}

int chown_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {		/* set object owner */
int ObjectID;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(*CommandTokens[1] != '#') {				/* not a valid ID number */
	SetLastError(currentuser,SYNTAX_ERROR);
	return(-1);
}

sscanf(CommandTokens[1],"#%d",&ObjectID);

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

if(*CommandTokens[1] != '#') {				/* not a valid ID number */
	SetLastError(currentuser,SYNTAX_ERROR);
	return(-1);
}

sscanf(CommandTokens[1],"#%d",&ObjectID);

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
				attributes &= OBJECT_DELETE_OWNER;
				break;

			case 'D':
				attributes &= OBJECT_DELETE_PUBLIC;
				break;

			case 'm':
				attributes &= OBJECT_MOVEABLE_OWNER;
				break;	

			case 'M':
				attributes &= OBJECT_MOVEABLE_PUBLIC;
				break;

			case 'p':
				attributes &= OBJECT_PICKUP_OWNER;
				break;

			case 'P':
				attributes &= OBJECT_PICKUP_PUBLIC;
				break;

			case 'r':
				attributes &= OBJECT_RENAME_OWNER;
				break;

			case 'R':
				attributes &= OBJECT_RENAME_PUBLIC;
				break;

			case 't':
				attributes &= OBJECT_TEMPORARY;
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

if(*CommandTokens[1] != '#') {				/* not a valid ID number */
	SetLastError(currentuser,SYNTAX_ERROR);
	return(-1);
}

sscanf(CommandTokens[1],"#%d",&ObjectID);

if(*CommandTokens[2] == '#') sscanf(CommandTokens[1],"#%d",&DestinationObjectID);	/* get destination ID */

if(CopyObject(currentuser,CommandTokens[1],ObjectID) == 0) return(DeleteObject(currentuser,ObjectID) == -1);

/* move player */

UserPtr=GetUserPointerByName(CommandTokens[1]);		/* find user */
if(UserPtr != NULL) {			/* found user */
	go(UserPtr,DestinationObjectID);

	SetLastError(currentuser,NO_ERROR);
	return(0);
}

/* copy room */

if((GetRoomPointer(DestinationObjectID) == NULL) || (GetRoomPointer(ObjectID) == NULL)) {	/* room not found */
	SetLastError(currentuser,BAD_ROOM);
	return(-1);
}

memcpy(GetRoomPointer(DestinationObjectID),GetRoomPointer(ObjectID),sizeof(room));

SetLastError(currentuser,NO_ERROR);
return(0);
}

int dig_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int CreateDirection=CreateRoom(currentuser,CommandTokens[1]);
char *CreateMessage[BUF_SIZE];

sprintf(CreateMessage,"A room has been created to the %s\r\n",DirectionsMessage[CreateDirection]);
send(currentuser->handle,CreateMessage,strlen(CreateMessage),0);

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

if(currentuser->status < WIZARD) {		/* can't do that */
	SetLastError(currentuser,NOT_YET);  
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
if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);  
	return(-1);
}

return(GetConfiguration());
}

int shutdown_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(ShutdownServer(currentuser,CommandTokens[1]));
}

int addclass_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
class class;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

strcpy(class.name,CommandTokens[1]);
	
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

strcpy(race.name,CommandTokens[1]);
race.magic=atoi(CommandTokens[2]);
race.strength=atoi(CommandTokens[3]);
race.agility=atoi(CommandTokens[4]);
race.luck=atoi(CommandTokens[5]);
race.wisdom=atoi(CommandTokens[6]);
race.intelligence=atoi(CommandTokens[7]);
race.stamina=atoi(CommandTokens[8]);

if(AddNewRace(&race) == -1) {
	SetLastError(currentuser,NO_MEM);
	return(-1);
}

return(0);
}

int dropdead_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(UpdateUser(currentuser,currentuser->name,"",0,0,"",0,0,0,0,"","",0));
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
int r;
int RoomLoop;

if(strcmp(CommandTokens[1],"here") == 0) {
	room=currentuser->room;
}
else
{
	room=atoi(CommandTokens[1]);
}

if(strcmp(CommandTokens[3],"here") == 0) {
	r=currentuser->room;
}
else
{
	r=atoi(CommandTokens[3]);
}

for(RoomLoop=0;RoomLoop<11;RoomLoop++) {
	if(strcmp(GetDirectionName(RoomLoop),CommandTokens[4]) == 0) break;
}

return(SetExit(currentuser,room,atoi(CommandTokens[2]),RoomLoop));
}

