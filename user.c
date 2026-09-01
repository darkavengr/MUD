#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdbool.h>

#define _XOPEN_SOURCE
#define _GNU_SOURCE

#ifdef __linux__
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef _WIN32
#include "windows.h"
#include <winsock2.h>

#include <ws2tcpip.h>
// #define inet_ntop InetNtop
#endif

#include "bool.h"
#include "help.h"
#include "ban.h"
#include "class.h"
#include "race.h"
#include "errors.h"
#include "user.h"
#include "config.h"
#include "world.h"
#include "getconfig.h"
#include "string.h"
#include "password.h"

user *LoggedInUsers=NULL;
user *LoggedInUsers_end=NULL;

/* titles */
char *MaleUserLevelNames[] = {"","Novice","Warrior","Hero","Champion","Superhero","Enchanter","Sorceror","Necromancer", \
			"Legend","Wizard","Arch Wizard","Dungeon Master" };

char *FemaleUserLevelNames[] = {"","Novice","Warrior","Heroine","Champion","Superheroine","Enchanteress","Sorceroress", \
			"Legend","Witch","Arch Witch","Dungeon Master" };

char *GoodbyeMessage="Goodbye.";
char *PlayAgainMessage="Play again (y/n)?";
int UserUpdated;

int ListBans(user *currentuser,char *ipaddress) {
char *IPAddress[BUF_SIZE];
char *reason[BUF_SIZE];
char *buf[BUF_SIZE];
sqlite3_stmt *SQLStatementHandle;
int returncode;

if(currentuser->userlevel < WIZARD) {             /* can't do this unless wizard or higher level */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

if(strlen(ipaddress) == 0) {		/* no IP address */
	strncpy(IPAddress,"%",BUF_SIZE);
}
else
{						
	WildcardToSQLWildcard(ipaddress,IPAddress);
}

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM BANS WHERE IPAddress LIKE ?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	/* prepare SQL statement */
	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

sqlite3_bind_text(SQLStatementHandle,1,IPAddress,strlen(IPAddress),NULL);		/* bind IP address to first parameter */

while(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {
	strncpy(IPAddress,sqlite3_column_text(SQLStatementHandle,BAN_IPADDRESS_COLUMN),BUF_SIZE);
	strncpy(reason,sqlite3_column_text(SQLStatementHandle,BAN_REASON_COLUMN),BUF_SIZE);

	sprintf(buf,"%s %s\r\n",IPAddress,reason);
	send(currentuser->socket,buf,strlen(buf),0);
}

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(0);
}

int BanUserByIPAddress(user *currentuser,char *ipaddress,char *reason) {
char *IPAddress[BUF_SIZE];
sqlite3_stmt *SQLStatementHandle;

if(currentuser->userlevel < WIZARD) {             /* can't do this unless wizard or higher level */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

/* prepare SQL statement */
if(sqlite3_prepare_v2(GetDatabaseHandle(),"INSERT INTO BANS (IPADDRESS,REASON) VALUES (?,?);",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

sqlite3_bind_text(SQLStatementHandle,1,ipaddress,strlen(ipaddress),NULL);		/* bind IP address to first parameter */
sqlite3_bind_text(SQLStatementHandle,2,reason,strlen(reason),NULL);		/* bind IP address to second parameter */

if(sqlite3_step(SQLStatementHandle) == SQLITE_DONE) {		/* inserted OK */
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(0);
}

SetLastError(currentuser,ALREADY_BANNED);

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(-1);
}

int UnBanUserByIPAddress(user *currentuser,char *ipaddress) {
char *IPAddress[BUF_SIZE];
char *reason[BUF_SIZE];
sqlite3_stmt *SQLStatementHandle;

if(currentuser->userlevel < WIZARD) {             /* can't do this unless wizard or higher level */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

if(sqlite3_prepare_v2(GetDatabaseHandle(),"DELETE FROM BANS WHERE IPAddress=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

sqlite3_bind_text(SQLStatementHandle,1,ipaddress,strlen(ipaddress),NULL);		/* bind IP address to first parameter */

if(sqlite3_step(SQLStatementHandle) == SQLITE_DONE) {		/* deleted OK */
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(0);
}

SetLastError(currentuser,OBJECT_NOT_FOUND);

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(-1);
}
	
/*
* force user to do something
*/

int ForceUser(user *currentuser,char *username,char *command) {
user *userptr;

if(currentuser->userlevel < WIZARD) {             /* can't do this unless wizard or higher level */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

userptr=LoggedInUsers;

while(userptr != NULL) {
	if(regexp(username,userptr->username) == TRUE) return(ExecuteCommand(userptr,command));    /* do command */ 

	userptr=userptr->next;
}

return(0);
}

/* give object to user */

int GiveObjectToUser(user *currentuser,char *objectname,char *username) {
user *userptr;
roomobject *RoomObjectPtr;
roomobject *temp;
bool UserFound=FALSE;
bool  ObjectFound=FALSE;
roomobject *ourobject;
roomobject *SaveObjectPtr;

/*
* find user
*/

userptr=LoggedInUsers;

while(userptr != NULL) {
	if(regexp(userptr->username,username) == TRUE) {		/* found object */
		UserFound=TRUE;
		break;
	}

	userptr=userptr->next;
}

if(UserFound == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

/*
* find object
*/

ourobject=currentuser->carryobjects;

while(ourobject != NULL) {
	if(regexp(ourobject->name,objectname) == TRUE) {		/* found object */

		ObjectFound=TRUE;

		if(SetInventoryObjectOwnerInDatabase(userptr->username,ourobject->name) == -1) return(-1);

		if(userptr->carryobjects == NULL) {				/* find end */
			userptr->carryobjects=calloc(1,sizeof(roomobject));
			if(RoomObjectPtr->next == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}

			userptr->carryobjects_last=userptr->carryobjects;
		}
		else
		{
			userptr->carryobjects_last->next=calloc(1,sizeof(roomobject));	/* allocate objects */ 
			if(userptr->carryobjects_last->next == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}
		}

		userptr->carryobjects_last=userptr->carryobjects_last->next;

		memcpy(userptr->carryobjects_last,ourobject,sizeof(roomobject));	/* copy data */

		/* remove object from source player's inventory */
		if(ourobject == currentuser->carryobjects) {		/* first object */
			SaveObjectPtr=ourobject;

			currentuser->carryobjects=currentuser->carryobjects->next;

			if(ourobject != NULL) ourobject->prev=NULL;

			free(SaveObjectPtr);   
		}
		else if(ourobject->next == NULL) {		/* last object */
			SaveObjectPtr=ourobject;

			ourobject->prev->next=NULL;

			free(SaveObjectPtr);
	
			currentuser->carryobjects_last->prev->next=NULL;				

			ourobject=ourobject->prev;
		}
		else
		{     
			ourobject->prev->next=ourobject->next;
			free(ourobject);

			ourobject=ourobject->prev;
		}
	}

	ourobject=ourobject->next;
}

if(ObjectFound == FALSE) {
	SetLastError(currentuser,OBJECT_NOT_FOUND);
	return(-1);
}

return(0);
}

/*
* kill user
*/
int KillUser(user *currentuser,char *username) {
room *roomnext;
user *userptr;
monster *monsternext;
char *OutputMessage[BUF_SIZE];
int found=FALSE;
int count;
room *currentroom;
sqlite3_stmt *SQLStatementHandle;
user *saveuserptr;
int KillSocket;

currentroom=currentuser->roomptr;

if(currentuser->userlevel < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

found=FALSE;

userptr=LoggedInUsers;
while(userptr != NULL) {
	if(regexp(userptr->username,username) == TRUE) {		/* found user */
		found=TRUE;

		if(currentuser->userlevel < userptr->userlevel ) {  /* wizards can't be killed */
			SetLastError(currentuser,KILL_WIZARD);
			return(-1);
		}

		if(userptr->gender == MALE) {
			sprintf(OutputMessage,"You were given the finger of death by %s the %s\r\n",currentuser->username,MaleUserLevelNames[currentuser->userlevel]);
		}
		else
		{
			sprintf(OutputMessage,"You were given the finger of death by %s the %s\r\n",currentuser->username,FemaleUserLevelNames[currentuser->userlevel]);
		}

		send(userptr->socket,OutputMessage,strlen(OutputMessage),0);

		KillSocket=userptr->socket;

		/* remove user from list of logged in users */

		if(userptr == LoggedInUsers) {		/* first user */
			saveuserptr=userptr;

			userptr=userptr->next;
			if(userptr != NULL) userptr->prev=NULL;
	
			free(saveuserptr);
			
		}
		else if(userptr->next == NULL) {		/* last user */
			saveuserptr=userptr;
			userptr->prev->next=NULL;

			free(saveuserptr);

			LoggedInUsers_end->prev->next=NULL;				
			userptr=userptr->prev;
		}
		else {
			userptr->prev->next=userptr->next;	/* skip over over object */
			free(userptr);

			userptr=userptr->prev;
		}

		/* delete user */
		if(sqlite3_prepare_v2(GetDatabaseHandle(),"DELETE FROM USERS WHERE USERNAME=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
			SetLastError(currentuser,IO_ERROR);
			return(-1);
		}

		sqlite3_bind_text(SQLStatementHandle,1,username,strlen(username),NULL);

		if(sqlite3_step(SQLStatementHandle) != SQLITE_DONE) {
			if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
			return(-1);
		}		

		/* Delete from inventory */

		if(sqlite3_prepare_v2(GetDatabaseHandle(),"DELETE FROM INVENTORY WHERE OWNER=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
			SetLastError(currentuser,IO_ERROR);
			return(-1);
		}

		sqlite3_bind_text(SQLStatementHandle,1,username,strlen(username),NULL);

		if(sqlite3_step(SQLStatementHandle) != SQLITE_DONE) {
			if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
			return(-1);
		}

		shutdown(KillSocket, SHUT_RDWR);		/* shutdown socket */
	}

	userptr=userptr->next;
}

/*
* if monster
*/

if(found == FALSE) {
	for(count=0;count<currentroom->monstercount;count++) {
		if(regexp(username,currentroom->roommonsters[count].name) == TRUE) {		/* found monster */
			DeleteMonster(currentroom->id,count);
			found=TRUE;
		}
	}
}

if(found == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER); /* unknown user */
	return(-1);
}

return(0);
}

/*
* send "emote" message
*/

int pose(user *currentuser,char *message) {
user *userptr;
char *OutputMessage[BUF_SIZE];

userptr=LoggedInUsers;

while(userptr != NULL) {

	if(userptr->room == currentuser->room) {	/* in same room */
		sprintf(OutputMessage,"*%s %s\r\n",currentuser->username,message);

		SendMessageToAllInRoom(userptr->room,OutputMessage);	/* send message */
	}

	userptr=userptr->next;
}

return(0);
}

/*
* quit
*/

void quit(user *currentuser) {
char *OutputMessage[BUF_SIZE];

sprintf(OutputMessage,"%s has disconnected\r\n",currentuser->username);

SendMessageToAllInRoom(currentuser->room,OutputMessage);

send(currentuser->socket,GoodbyeMessage,strlen(GoodbyeMessage),0);

DisconnectUser(currentuser,currentuser->username);		/* disconnect user */
}

/*
* send private message to someone
*/

int SendMessageToAllInRoom(int room,char *message) {
user *userptr;

userptr=LoggedInUsers;

while(userptr != NULL) {
	if(userptr->room == room) send(userptr->socket,message,strlen(message),0);	/* found user */

	userptr=userptr->next;
}

return(0);
}

/*
* send private message to someone
*/

int SendMessage(user *currentuser,char *username,char *message) {
char *OutputMessage[BUF_SIZE];
user *userptr;
int found=FALSE;

userptr=LoggedInUsers;

while(userptr != NULL) {
	if(regexp(username,userptr->username) == TRUE) {		/* found user */

		if(currentuser->flags & USER_INVISIBLE) {
			sprintf(OutputMessage,"Somebody whispers, %s\r\n",username,message);
		}
		else
		{
			sprintf(OutputMessage,"[%s] %s\r\n",username,message);
		}

		found=TRUE;
		send(currentuser->socket,OutputMessage,strlen(OutputMessage),0);
	}

	userptr=userptr->next;
}

if(found == FALSE) {			/* unknown user */
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(0);
}

/*
* take object from user
*/


int TakeObject(user *currentuser,char *objectname,char *username) {
user *userptr;
roomobject *RoomObjectPtr;
roomobject *temp;
bool UserFound=FALSE;
bool  ObjectFound=FALSE;
roomobject *theirobject;
roomobject *SaveObjectPtr;

if(currentuser->userlevel < WIZARD) {             /* can't do this unless wizard or higher level */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

/*
* find user
*/

userptr=LoggedInUsers;

while(userptr != NULL) {
	if(regexp(userptr->username,username) == TRUE) {		/* found object */
		UserFound=TRUE;
		break;
	}

	userptr=userptr->next;
}

if(UserFound == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

/*
* find object
*/

theirobject=userptr->carryobjects;

while(theirobject != NULL) {
	if(regexp(theirobject->name,objectname) == TRUE) {		/* found object */
		ObjectFound=TRUE;

		if(SetInventoryObjectOwnerInDatabase(userptr->username,theirobject->name) == -1) return(-1);

		if(currentuser->carryobjects == NULL) {				/* find end */
			currentuser->carryobjects=calloc(1,sizeof(roomobject));
			if(currentuser->carryobjects == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}

			currentuser->carryobjects_last=currentuser->carryobjects;
		}
		else
		{
			currentuser->carryobjects_last->next=calloc(1,sizeof(roomobject));	/* allocate objects */ 
			if(currentuser->carryobjects_last->next == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}

			currentuser->carryobjects_last=currentuser->carryobjects_last->next;
		}

		memcpy(currentuser->carryobjects_last,theirobject,sizeof(roomobject));	/* copy data */

		/* remove object from the source player's inventory */
		if(theirobject == userptr->carryobjects) {		/* first object */
			SaveObjectPtr=theirobject;

			theirobject=theirobject->next;

			if(theirobject != NULL) theirobject->prev=NULL;

			free(SaveObjectPtr);   
		}
		else if(theirobject->next == NULL) {		/* last object */
			SaveObjectPtr=theirobject;

			theirobject->prev->next=NULL;

			free(SaveObjectPtr);
	
			currentuser->carryobjects_last->prev->next=NULL;				

			theirobject=theirobject->prev;
		}
		else
		{     
			theirobject->prev->next=theirobject->next;
			free(theirobject);

			theirobject=theirobject->prev;
		}
	}

	theirobject=theirobject->next;
}

if(ObjectFound == FALSE) {
	SetLastError(currentuser,OBJECT_NOT_FOUND);
	return(-1);
}

return(0);
}


/*
* update user info
*/

int UpdateUser(user *currentuser,char *username,char *newusername,char *password,int homeroom,int userlevel,char *description,int magicpoints,int staminapoints,int experiencepoints,int gender,char *racex,char *classx,int flags) {
int dead=0;
int count;
user *userptr;
roomobject *RoomObjectPtr;
char *OutputMessage[BUF_SIZE];
race newrace;
userclass newclass;
CONFIG config;
sqlite3_stmt *SQLStatementHandle;

if(homeroom < 0) homeroom=0;			/* sanity check */
if(userlevel < 0) userlevel=0;
if(magicpoints < 0) magicpoints=0;
if(staminapoints < 0) staminapoints=0;
if(experiencepoints < 0) experiencepoints=0;

GetConfigurationInformation(&config);

userptr=LoggedInUsers;
while(userptr != NULL) {

/* If the stamina points are 0 the user is killed */

	if(regexp(userptr->username,username) == TRUE) {			/* found user */
		strncpy(userptr->username,newusername,BUF_SIZE);
		strncpy(userptr->password,crypt(password,newusername),BUF_SIZE);
		strncpy(userptr->description,description,BUF_SIZE);
		userptr->userlevel=userlevel;
		userptr->gender=gender;
		userptr->homeroom=homeroom;
		userptr->magicpoints=magicpoints;
		userptr->staminapoints=staminapoints;
		userptr->experiencepoints=experiencepoints;

		if(GetRace(racex,&newrace) == -1) {		/* update race */
			SetLastError(currentuser,INVALID_RACE);
			return(-1);
		}

		strncpy(userptr->race.name,&newrace.name,BUF_SIZE);

		if(GetClass(classx,&newclass) == -1) {		/* update class */
			SetLastError(currentuser,INVALID_CLASS);
			return(-1);
		}

		strncpy(userptr->userclass.name,&newclass.name,BUF_SIZE);
		userptr->flags=flags;


		/*
		* the user is dead, long live the user
		*/

		if(userptr->staminapoints <= 0 && (userptr->userlevel < WIZARD)) {

		}

	}

	userptr=userptr->next;
}


/* Update user */

/* prepare statement SQL */
if(sqlite3_prepare_v2(GetDatabaseHandle(),"UPDATE USERS SET username=?,password=?,homeroom=?,userlevel=?,desc=?,magicpoints=?,staminapoints=?,experiencepoints=?,gender=?,race=?,class=?,flags=? WHERE username=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

/* bind parameters */
sqlite3_bind_text(SQLStatementHandle,1,newusername,strlen(newusername),NULL);
sqlite3_bind_text(SQLStatementHandle,2,password,strlen(password),NULL);
sqlite3_bind_int(SQLStatementHandle,3,homeroom);
sqlite3_bind_int(SQLStatementHandle,4,userlevel);
sqlite3_bind_text(SQLStatementHandle,5,description,strlen(description),NULL);
sqlite3_bind_int(SQLStatementHandle,6,magicpoints);
sqlite3_bind_int(SQLStatementHandle,7,staminapoints);
sqlite3_bind_int(SQLStatementHandle,8,experiencepoints);
sqlite3_bind_int(SQLStatementHandle,9,gender);
sqlite3_bind_text(SQLStatementHandle,10,racex,strlen(racex),NULL);
sqlite3_bind_text(SQLStatementHandle,11,classx,strlen(classx),NULL);
sqlite3_bind_int(SQLStatementHandle,12,flags);
sqlite3_bind_text(SQLStatementHandle,13,username,strlen(username),NULL);

if(sqlite3_step(SQLStatementHandle) == SQLITE_DONE) {		/* update OK */
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(0);
}

SetLastError(currentuser,UNKNOWN_USER);

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(-1);
}

/*
* set user points (magic/stamina/experience)
*/

int SetUserPoints(user *currentuser,char *username,char *amountstr,int which) {
user *userptr;
int amount;

userptr=LoggedInUsers;

while(userptr != NULL) {

	if(regexp(userptr->username,username) == TRUE) {	/* if user found */

		/* adding/subtracting/setting points */

		if((char) *amountstr == '+') {
			sscanf(amountstr,"+%d",&amount);
		}
		else if((char) *amountstr == '-') {
			sscanf(amountstr,"-%d",&amount);
		}
		else {			
			amount=atoi(amountstr);
		}
	
		if(which == MAGICPOINTS) {
			return(UpdateUser(currentuser,userptr->username,userptr->username,userptr->password,userptr->homeroom,userptr->userlevel,userptr->description,userptr->magicpoints+amount,userptr->staminapoints,userptr->experiencepoints,userptr->gender,userptr->race.name,userptr->userclass.name,userptr->flags));
		}
		else if(which == STAMINAPOINTS) {
			return(UpdateUser(currentuser,userptr->username,userptr->username,userptr->password,userptr->homeroom,userptr->userlevel,userptr->description,userptr->magicpoints,userptr->staminapoints+amount,userptr->experiencepoints,userptr->gender,userptr->race.name,userptr->userclass.name,userptr->flags));
		}
		else if(which == EXPERIENCEPOINTS) {
			return(UpdateUser(currentuser,userptr->username,userptr->username,userptr->password,userptr->homeroom,userptr->userlevel,userptr->description,userptr->magicpoints+amount,userptr->staminapoints,userptr->experiencepoints+amount,userptr->gender,userptr->race.name,userptr->userclass.name,userptr->flags));
		}
	}

	userptr=userptr->next;
}

SetLastError(currentuser,UNKNOWN_USER);		/* user not found */
return(-1);
}

/*
* set user level */

int SetUserLevel(user *currentuser,char *username,char *levelstr) {
user *userptr;
int level;

if(currentuser->userlevel < WIZARD) {     /* not wizard */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

userptr=LoggedInUsers;
while(userptr != NULL) {

	if(regexp(userptr->username,username) == TRUE) {		/* found user */
	
		if((char) *levelstr == '+') {			/* add points */
			sscanf(levelstr,"+%d",&level);

			if(userptr->userlevel+level > 12) {
				SetLastError(currentuser,INVALID_LEVEL);
				return(-1);
			}
	
			 if(userptr->userlevel+level > currentuser->userlevel) {		/* can't set level above own level */
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			 }

			return(UpdateUser(currentuser,userptr->username,userptr->username,userptr->password,userptr->homeroom,userptr->userlevel+level,userptr->description,userptr->magicpoints,userptr->staminapoints,userptr->experiencepoints,userptr->gender,userptr->race.name,userptr->userclass.name,userptr->flags));
		}
		else if((char) *levelstr == '-') {			/* subtract points */
			sscanf(levelstr,"-%d",&level);

			if((userptr->userlevel-level <= 0) || (userptr->userlevel+level <= 0)) {
				SetLastError(currentuser,INVALID_LEVEL);
				return(-1);
			}

			return(UpdateUser(currentuser,userptr->username,userptr->username,userptr->password,userptr->homeroom,userptr->userlevel,userptr->description,userptr->magicpoints,userptr->staminapoints,userptr->experiencepoints,userptr->gender,userptr->race.name,userptr->userclass.name,userptr->flags));	
		}
		else {
			level=atoi(levelstr);

			if((level > 12) || (level <= 0)) {
				SetLastError(currentuser,INVALID_LEVEL);
				return(-1);
			}

			return(UpdateUser(currentuser,userptr->username,userptr->username,userptr->password,userptr->homeroom,level,userptr->description,userptr->magicpoints,userptr->staminapoints,userptr->experiencepoints,userptr->gender,userptr->race.name,userptr->userclass.name,userptr->flags));
		}
	}

	userptr=userptr->next;
}

return(0);
}


/* set gender */
int SetUserGender(user *currentuser,char *username,char *gender) {
user *userptr;

if(currentuser->userlevel < WIZARD) {		/* can't do this yet */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

userptr=LoggedInUsers;

while(userptr != NULL) {
	if(strncmp(userptr->username,username,BUF_SIZE) == 0) {

		if(strncmp(gender,"male",BUF_SIZE) == 0) {
			return(UpdateUser(currentuser,userptr->username,userptr->username,userptr->password,userptr->homeroom,userptr->userlevel,userptr->description,userptr->magicpoints,userptr->staminapoints,userptr->experiencepoints,MALE,userptr->race.name,userptr->userclass.name,userptr->flags));
		}
		else if(strncmp(gender,"female",BUF_SIZE) == 0) {
			return(UpdateUser(currentuser,userptr->username,userptr->username,userptr->password,userptr->homeroom,userptr->userlevel,userptr->description,userptr->magicpoints,userptr->staminapoints,userptr->experiencepoints,FEMALE,userptr->race.name,userptr->userclass.name,userptr->flags));
		}

		userptr=userptr->next;
	}
}

SetLastError(currentuser,INVALID_GENDER);
return(-1);
}


int SetVisibleMode(user *currentuser,char *name,int mode) {
user *next=LoggedInUsers;

if(currentuser->userlevel < WIZARD) {     /* not wizard */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

while(next != NULL) {
	if(strncmp(next->username,name,BUF_SIZE) == 0) {

		if(mode == 0) {			/* go visible */
			next->flags &= USER_INVISIBLE;
		}
		else
		{
			next->flags &= ~USER_INVISIBLE;
		}

		return(0);
	}

	next=next->next;
}

SetLastError(currentuser,UNKNOWN_USER);
return(-1);
}

int GagUser(user *currentuser,char *name,int mode) {
user *next=LoggedInUsers;

if(currentuser->userlevel < WIZARD) {     /* not wizard */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

if(*name) {
	next=currentuser;
}
else
{
	while(next != NULL) {
		if(strncmp(next->username,name,BUF_SIZE) == 0) break;

		next=next->next;
	}

	if(next == NULL) {
		SetLastError(currentuser,UNKNOWN_USER);
		return(-1);
	}
}

if(mode == 0) {		
	next->flags &= USER_GAGGED;
}
else
{
	next->flags |= USER_GAGGED;
}

return(0);
}

/*
* send message to everone connected
*/

int wall(user *currentuser,char *message) {
user *userptr;
char *OutputMessage[BUF_SIZE];

if(currentuser->userlevel < WIZARD) {		/* only wizard or higher users can send global message */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

userptr=LoggedInUsers;

while(userptr != NULL) {
	sprintf(OutputMessage,"[GLOBAL MESSAGE] %s\n",message);
	send(userptr->socket,OutputMessage,strlen(OutputMessage),0);			/* send message to every user */

	userptr=userptr->next;
}

return(0);
}

int go(user *currentuser,int RoomNumber) {
room *roomnext;
char *OutputMessage[BUF_SIZE];

if(RoomNumber == 0) {		/* invalid room */
	SetLastError(currentuser,INVALID_DIRECTION);
	return(-1);
}

if(currentuser->room != RoomNumber) {	/* send leaving message */
	sprintf(OutputMessage,"%s has left\r\n",currentuser->username);
	SendMessageToAllInRoom(currentuser->room,OutputMessage);
}

if(GetRoomAttributes(RoomNumber) & ROOM_PRIVATE) {
	SetLastError(currentuser,INVALID_DIRECTION);
	return(-1);
}	

strncpy(currentuser->roomname,GetRoomName(RoomNumber),BUF_SIZE);

currentuser->room=RoomNumber;
currentuser->roomptr=GetRoomPointer(RoomNumber); 		/* save pointer to current room */

sprintf(OutputMessage,"%s has entered\r\n",currentuser->username);
SendMessageToAllInRoom(currentuser->room,OutputMessage);

look_command(currentuser,0,NULL);				/* look at new room */

if(GetRoomAttributes(RoomNumber) & ROOM_DEAD) {
	UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,currentuser->userlevel,currentuser->description,currentuser->magicpoints,0,currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags);
	return(-1);
}	

return(0);
}

int MoveObject(user *currentuser,char *ObjectName,int RoomNumber) {
roomobject *RoomObjectPtr;
roomobject *DestinationObject;
int destination;
user *userptr;
room *currentroom;
room *DestinationRoom;
int FoundRoom=FALSE;
int found=FALSE;
CONFIG config;

GetConfigurationInformation(&config);

currentroom=currentuser->roomptr;

if(currentuser->userlevel < WIZARD) {      /* not wizard */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

if(RoomNumber > GetNumberOfRooms()) {			/* can't find room */
	SetLastError(currentuser,OBJECT_NOT_FOUND);
	return(-1);
}

DestinationRoom=GetRoomPointer(RoomNumber);		/* point to room */

RoomObjectPtr=currentroom->roomobjects;

while(RoomObjectPtr != NULL) {
	if(regexp(RoomObjectPtr->name,ObjectName) == 0 ) {				/* if object matches */

		if(currentuser->userlevel < ARCHWIZARD) {
			if((strncmp(currentroom->owner,currentuser->username,BUF_SIZE) == 0) && (currentroom->attributes & OBJECT_MOVEABLE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			if((strncmp(currentroom->owner,currentuser->username,BUF_SIZE) == 0) && (currentroom->attributes & OBJECT_MOVEABLE_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		if(DestinationRoom->roomobjects != NULL) {				/* find end */
			DestinationObject=DestinationRoom->roomobjects;

			while(DestinationObject->next != NULL) DestinationObject=DestinationObject->next; 

			DestinationObject->next=calloc(1,sizeof( roomobject));	/* allocate objects */ 

			if(DestinationObject->next == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}

			DestinationObject=DestinationObject->next;
		}
		else
		{
			DestinationRoom->roomobjects=calloc(1,sizeof( roomobject));	/* allocate objects */ 
			DestinationObject=DestinationRoom->roomobjects;

			if(DestinationObject == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}
		}

		memcpy(DestinationObject,RoomObjectPtr,sizeof( roomobject));		/* copy object */
		DeleteObject(currentuser,ObjectName);                                  /* delete object */

		found=TRUE;
	}

	RoomObjectPtr=RoomObjectPtr->next;
}

return(0);
}

int LoginUser(int messagesocket,char *username,char *password) {
int ErrorCount=0;
char *hashedpassword[BUF_SIZE];
struct sockaddr_in clientip;
socklen_t clientiplen;
char *ipaddress[BUF_SIZE];
sqlite3_stmt *SQLStatementHandle;
roomobject *carryptr=NULL;
char *temp[BUF_SIZE];
int returnvalue;
char *classname[BUF_SIZE];
char *racename[BUF_SIZE];
int returncode;

clientiplen=sizeof(struct sockaddr_in);			/* get IP address */
getpeername(messagesocket,(struct sockaddr*)&clientip,&clientiplen);

strncpy(ipaddress,inet_ntoa(clientip.sin_addr),BUF_SIZE);

if(HashPassword(username,password,hashedpassword) == -1) return(-1);	/* hash password */	

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM USERS WHERE USERNAME=? AND PASSWORD=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	/* prepare SQL statement */
	return(-1);
}

/* bind parameters */
sqlite3_bind_text(SQLStatementHandle,1,username,strlen(username),NULL);
sqlite3_bind_text(SQLStatementHandle,2,hashedpassword,strlen(hashedpassword),NULL);

returncode=sqlite3_step(SQLStatementHandle);

if(returncode == SQLITE_ERROR) {
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(-1);
}

if(returncode == SQLITE_ROW) {
	/* get user information */

	if(LoggedInUsers == NULL) {		/* no logged in users */
		LoggedInUsers=malloc(sizeof(user));
		if(LoggedInUsers == NULL) return(-1);

		LoggedInUsers->prev=NULL;
		LoggedInUsers_end=LoggedInUsers;
	}
	else
	{
		LoggedInUsers_end->next=malloc(sizeof(user));
		if(LoggedInUsers_end->next == NULL) return(-1);		/* allocation error */

		LoggedInUsers_end->next->prev=LoggedInUsers_end;	/* previous user */

		LoggedInUsers_end=LoggedInUsers_end->next;
	}

	strncpy(LoggedInUsers_end->username,sqlite3_column_text(SQLStatementHandle,USERNAME),BUF_SIZE);
	strncpy(LoggedInUsers_end->password,sqlite3_column_text(SQLStatementHandle,PASSWORD),BUF_SIZE);
	strncpy(LoggedInUsers_end->description,sqlite3_column_text(SQLStatementHandle,DESCRIPTION),BUF_SIZE);
	LoggedInUsers_end->userlevel=sqlite3_column_int(SQLStatementHandle,USERLEVEL);
	LoggedInUsers_end->gender=sqlite3_column_int(SQLStatementHandle,GENDER);
	LoggedInUsers_end->homeroom=sqlite3_column_int(SQLStatementHandle,HOMEROOM);
	LoggedInUsers_end->magicpoints=sqlite3_column_int(SQLStatementHandle,MAGICPOINTS);
	LoggedInUsers_end->staminapoints=sqlite3_column_int(SQLStatementHandle,STAMINAPOINTS);
	LoggedInUsers_end->experiencepoints=sqlite3_column_int(SQLStatementHandle,EXPERIENCEPOINTS);
	strncpy(racename,sqlite3_column_text(SQLStatementHandle,RACE),BUF_SIZE);
	strncpy(classname,sqlite3_column_text(SQLStatementHandle,CLASS),BUF_SIZE);
	LoggedInUsers_end->flags=sqlite3_column_int(SQLStatementHandle,USERFLAGS);
	LoggedInUsers_end->next=NULL;

	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);

	if(GetRace(racename,&LoggedInUsers_end->race) == -1) return(-1);	/* get user race */
	if(GetClass(classname,&LoggedInUsers_end->userclass) == -1) return(-1);	/* get user class */

	/*
	* load user inventory
	*/

	LoggedInUsers_end->carryobjects=NULL;

	if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM INVENTORY WHERE OWNER=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	/* prepare SQL statement */
		return(-1);
	}

	/* bind parameters */
	sqlite3_bind_text(SQLStatementHandle,1,username,strlen(username),NULL);

	while(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {
	
		if(LoggedInUsers_end->carryobjects == NULL) {
			LoggedInUsers_end->carryobjects=calloc(1,sizeof(roomobject));		/* allocate objects */
			if(LoggedInUsers_end->carryobjects == NULL) return(-1);		/* can't allocate */

			LoggedInUsers_end->carryobjects_last=LoggedInUsers_end->carryobjects;
			LoggedInUsers_end->carryobjects_last->prev=NULL;

		}
		else
		{
			LoggedInUsers_end->carryobjects_last->next=calloc(1,sizeof(roomobject));		/* allocate objects */
			if(LoggedInUsers_end->carryobjects_last->next == NULL) return(-1);		/* can't allocate */

			LoggedInUsers_end->carryobjects_last->next->prev=LoggedInUsers_end->carryobjects_last;

			LoggedInUsers_end->carryobjects_last=LoggedInUsers_end->carryobjects_last->next;
		}

		strncpy(LoggedInUsers_end->carryobjects_last->name,sqlite3_column_text(SQLStatementHandle,INVOBJECT_NAME),BUF_SIZE);
		LoggedInUsers_end->carryobjects_last->attributes=sqlite3_column_int(SQLStatementHandle,INVOBJECT_ATTRIBUTES);
		strncpy(LoggedInUsers_end->carryobjects_last->description,sqlite3_column_text(SQLStatementHandle,INVOBJECT_DESCRIPTION),BUF_SIZE);
		strncpy(LoggedInUsers_end->carryobjects_last->owner,sqlite3_column_text(SQLStatementHandle,INVOBJECT_OWNER),BUF_SIZE);

		LoggedInUsers_end->carryobjects_last->next=NULL;
	}

	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);

	if(go(LoggedInUsers_end,LoggedInUsers_end->homeroom) == -1) {
		return(-1);
	}

	return(0);
}

return(-1);
}

int CreateUser(int socket,char *username,char *password,char *description,int userlevel,int gender,int homeroom,int magicpoints,int staminapoints,int experiencepoints,char *racex,char *classx,int flags) {
struct sockaddr_in clientip;
socklen_t clientiplen;
char *ipaddress[BUF_SIZE];
sqlite3_stmt *SQLStatementHandle;
char *hashedpassword[BUF_SIZE];

if(HashPassword(username,password,hashedpassword) == -1) return(-1);	/* hash password */	

if(sqlite3_prepare_v2(GetDatabaseHandle(),"INSERT INTO USERS (username,password,homeroom,userlevel,desc,magicpoints,staminapoints,experiencepoints,gender,race,class,flags) VAlUES (?,?,?,?,?,?,?,?,?,?,?,?);",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	printf("mud: %s\n",sqlite3_errmsg(GetDatabaseHandle()));
	return(-1);
}

/* bind parameters */

sqlite3_bind_text(SQLStatementHandle,1,username,strlen(username),NULL);
sqlite3_bind_text(SQLStatementHandle,2,hashedpassword,strlen(hashedpassword),NULL);
sqlite3_bind_int(SQLStatementHandle,3,homeroom);
sqlite3_bind_int(SQLStatementHandle,4,userlevel);
sqlite3_bind_text(SQLStatementHandle,5,description,strlen(description),NULL);
sqlite3_bind_int(SQLStatementHandle,6,magicpoints);
sqlite3_bind_int(SQLStatementHandle,7,staminapoints);
sqlite3_bind_int(SQLStatementHandle,8,experiencepoints);
sqlite3_bind_int(SQLStatementHandle,9,gender);
sqlite3_bind_text(SQLStatementHandle,10,racex,strlen(racex),NULL);
sqlite3_bind_text(SQLStatementHandle,11,classx,strlen(classx),NULL);
sqlite3_bind_int(SQLStatementHandle,12,flags);

if(sqlite3_step(SQLStatementHandle) != SQLITE_DONE) {
	printf("mud: %s\n",sqlite3_errmsg(GetDatabaseHandle()));
	
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(-1);
}

return(0);
}

int AddNewRace(user *currentuser,race *newrace) {
sqlite3_stmt *SQLStatementHandle;

if(currentuser->userlevel < WIZARD) {		/* can't do that */
	SetLastError(currentuser,ACCESS_DENIED);  
	return(-1);
}

/* prepare SQL statement */

if(sqlite3_prepare_v2(GetDatabaseHandle(),"INSERT INTO RACES (name,magic,strength,agility,dexterity,luck,wisdom,intelligence,stamina) VAlUES (?,?,?,?,?,?, ?,?,?);",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

/* bind parameters */
sqlite3_bind_text(SQLStatementHandle,1,newrace->name,strlen(newrace->name),NULL);
sqlite3_bind_int(SQLStatementHandle,2,newrace->magic);
sqlite3_bind_int(SQLStatementHandle,3,newrace->strength);
sqlite3_bind_int(SQLStatementHandle,4,newrace->agility);
sqlite3_bind_int(SQLStatementHandle,5,newrace->dexterity);
sqlite3_bind_int(SQLStatementHandle,6,newrace->luck);
sqlite3_bind_int(SQLStatementHandle,7,newrace->wisdom);
sqlite3_bind_int(SQLStatementHandle,8,newrace->intelligence);
sqlite3_bind_int(SQLStatementHandle,9,newrace->stamina);

if(sqlite3_step(SQLStatementHandle) != SQLITE_DONE) {
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(-1);
}

return(0);
}

int AddNewClass(user *currentuser,userclass *newclass) {
sqlite3_stmt *SQLStatementHandle;

if(currentuser->userlevel < WIZARD) {		/* can't do that */
	SetLastError(currentuser,ACCESS_DENIED);  
	return(-1);
}

/* prepare SQL statement */

if(sqlite3_prepare_v2(GetDatabaseHandle(),"INSERT INTO CLASSES (name) VAlUES (?);",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

/* bind parameters */
sqlite3_bind_text(SQLStatementHandle,1,newclass->name,strlen(newclass->name),NULL);

if(sqlite3_step(SQLStatementHandle) != SQLITE_DONE) {
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(-1);
}

SetLastError(currentuser,NO_ERROR);
return(0);
}

user *GetUserPointerByName(char *name) {
user *userptr;

userptr=LoggedInUsers;

while(userptr != NULL) {

	if(regexp(userptr->username,name) == TRUE) return(userptr);		/* found user */

	userptr=userptr->next;
}

return(NULL);
}

user *FindFirstUser(void) {
return(LoggedInUsers);
}

user *FindNextUser(user *last) {
return(last->next);
}

char *GetPointerToMaleTitles(int level) {
return(MaleUserLevelNames[level]);
}

char *GetPointerToFemaleTitles(int level) {
return(FemaleUserLevelNames[level]);
}

void AttackUser(int RoomNumber,int roommonster) {
user *userptr=LoggedInUsers;
int HitPoints;
char *OutputMessage[BUF_SIZE];

while(userptr != NULL) {
	if(userptr->room == RoomNumber) {		/* user is in room */
		HitPoints=rand() % (GetRoomMonsterEvil(RoomNumber,roommonster) + 1) - 0;		/* random damage */

		sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",GetRoomMonsterName(RoomNumber,roommonster),userptr->username,HitPoints);
		SendMessageToAllInRoom(RoomNumber,OutputMessage);

		userptr->staminapoints -= HitPoints;

		UpdateUser(userptr,userptr->username,userptr->username,userptr->password,userptr->homeroom,userptr->userlevel,userptr->description,userptr->magicpoints,userptr->staminapoints,userptr->experiencepoints,userptr->gender,userptr->race.name,userptr->userclass.name,userptr->flags);
	}

	userptr=userptr->next;
}

return;
}


int PickUpObject(user *currentuser,char *ObjectName) {
roomobject *UserCarryObjectsPtr;
char *ErrorMessage[BUF_SIZE];
sqlite3_stmt *SQLStatementHandle;
roomobject *roomobjectptr=currentuser->roomptr->roomobjects;
roomobject *SaveObject;

/* check if user is already carrying this object */
UserCarryObjectsPtr=currentuser->carryobjects;

while(UserCarryObjectsPtr != NULL) {
	if(strncmp(UserCarryObjectsPtr->name,ObjectName,BUF_SIZE) == 0) {	/* already picked up object */

		SetLastError(currentuser,ALREADY_HAVE_OBJECT);
		return(-1);
	}

	UserCarryObjectsPtr=UserCarryObjectsPtr->next;
}

/* add object to inventory */

if(currentuser->carryobjects == NULL) {
	currentuser->carryobjects=calloc(1,sizeof(roomobject));		/* add link to end */
	if(currentuser->carryobjects == NULL) {		/* can't allocate */
		SetLastError(currentuser,NO_MEM);
		return(-1);
	}

	currentuser->carryobjects_last=currentuser->carryobjects;
}
else
{  
	currentuser->carryobjects_last->next=calloc(1,sizeof(roomobject));		/* add link to end */
	if(currentuser->carryobjects_last->next == NULL) {		/* can't allocate */
		SetLastError(currentuser,NO_MEM);
		return(-1);
	}

	currentuser->carryobjects_last=currentuser->carryobjects_last->next;
}

strncpy(currentuser->carryobjects_last->name,ObjectName,BUF_SIZE);		/* add item */

if(currentuser->carryobjects_last->magicpoints > 0) {
	sprintf(ErrorMessage,"You have gained %d magic points!\r\n",currentuser->carryobjects_last->magicpoints);
	send(currentuser->socket,ErrorMessage,strlen(ErrorMessage),0);
}

if(currentuser->carryobjects_last->staminapoints > 0) {
	sprintf(ErrorMessage,"You have gained %d stamina points!\r\n",currentuser->carryobjects_last->staminapoints);
	send(currentuser->socket,ErrorMessage,strlen(ErrorMessage),0);
}  

currentuser->carryobjects_last->magicpoints=0;
currentuser->carryobjects_last->staminapoints=0;
currentuser->carryobjects_last->next=NULL;

/* delete from room */
while(roomobjectptr != NULL) {
	if(roomobjectptr->id == currentuser->carryobjects_last->id) {		/* found object */
		if(roomobjectptr == currentuser->roomptr->roomobjects) {	/* first */
			SaveObject=roomobjectptr;

			roomobjectptr=roomobjectptr->next;

			if(roomobjectptr != NULL) roomobjectptr->prev=NULL;

			free(SaveObject);
		}
		else if(roomobjectptr->next == NULL) {				/* last */
			SaveObject=roomobjectptr;

			roomobjectptr->prev->next=NULL;

			free(SaveObject);
		}																																																																											
		else
		{
			roomobjectptr->prev->next=roomobjectptr->next;
			currentuser->roomptr->roomobjects_last->prev->next=NULL;

			free(roomobjectptr);
		}

		break;
	}

	roomobjectptr=roomobjectptr->next;
}
	
/* update database */

if(sqlite3_prepare_v2(GetDatabaseHandle(),"INSERT INTO INVENTORY VALUES (?,?,?,?);",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	printf("mud: %s\n",sqlite3_errmsg(GetDatabaseHandle()));

	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

sqlite3_bind_int(SQLStatementHandle,1,currentuser->carryobjects_last->id);
sqlite3_bind_text(SQLStatementHandle,2,currentuser->carryobjects_last->name,strlen(currentuser->carryobjects_last->name),NULL);
sqlite3_bind_int(SQLStatementHandle,3,currentuser->carryobjects_last->attributes);
sqlite3_bind_text(SQLStatementHandle,4,currentuser->carryobjects_last->description,strlen(currentuser->carryobjects_last->description),NULL);
sqlite3_bind_text(SQLStatementHandle,5,currentuser->carryobjects_last->owner,strlen(currentuser->carryobjects_last->owner),NULL);

if(sqlite3_step(SQLStatementHandle) != SQLITE_DONE) {
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(-1);
}

SetLastError(currentuser,NO_ERROR);
return(0);
}

int DropObject(user *currentuser,char *ObjectName) {
roomobject *UserCarryObjectsPtr;
room *CurrentRoom;
char *ObjectsList[BUF_SIZE];
int found=FALSE;
roomobject *SaveObjectPtr;

/* check permissions */

CurrentRoom=currentuser->roomptr;

if(currentuser->userlevel < ARCHWIZARD) {
	if((CurrentRoom->attributes & ROOM_CREATE_OWNER) == 0) {
		SetLastError(currentuser,CANT_CREATE_OBJECTS_HERE);  
		return(-1);
	}
	else
	{
		if((CurrentRoom->attributes & ROOM_CREATE_PUBLIC) == 0) {
			SetLastError(currentuser,CANT_CREATE_OBJECTS_HERE);  
			return(-1);
		}
	}
}

UserCarryObjectsPtr=currentuser->carryobjects;		/* point to carried objects */

while(UserCarryObjectsPtr != NULL) {
	if(UserCarryObjectsPtr == NULL) break;

	if(regexp(UserCarryObjectsPtr->name,ObjectName) == TRUE) {	/* found object */
		found=TRUE;

		/* add to list of objects in room */

		if(CurrentRoom->roomobjects == NULL) {					       
			CurrentRoom->roomobjects=calloc(1,sizeof(roomobject));	/* allocate objects */
			if(CurrentRoom->roomobjects == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);  
				return(-1);
			}

			CurrentRoom->roomobjects_last=CurrentRoom->roomobjects;
		}
		else
		{
			CurrentRoom->roomobjects_last->next=calloc(1,sizeof(roomobject));	/* allocate objects */
			if(CurrentRoom->roomobjects_last->next == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);  
				return(-1);
			}

			CurrentRoom->roomobjects_last=CurrentRoom->roomobjects_last->next;
		}

		memcpy(CurrentRoom->roomobjects_last,UserCarryObjectsPtr,sizeof(roomobject));	/* copy object */
		CurrentRoom->roomobjects_last->id=GetNextObjectNumber();	/* generate new ID number for dropped object */
		
		/* remove object from inventory */

		if(UserCarryObjectsPtr == currentuser->carryobjects) {		/* first object */
			SaveObjectPtr=UserCarryObjectsPtr;

			UserCarryObjectsPtr=UserCarryObjectsPtr->next;
			if(UserCarryObjectsPtr != NULL) UserCarryObjectsPtr->prev=NULL;
	
			free(SaveObjectPtr);
			
		}
		else if(UserCarryObjectsPtr->next == NULL) {		/* last object */
			SaveObjectPtr=UserCarryObjectsPtr;

			UserCarryObjectsPtr->prev->next=NULL;
			free(SaveObjectPtr);

			currentuser->carryobjects_last->prev->next=NULL;				
			UserCarryObjectsPtr=UserCarryObjectsPtr->prev;
		}
		else {
			UserCarryObjectsPtr->prev->next=UserCarryObjectsPtr->next;	/* skip over over object */
			free(UserCarryObjectsPtr);

			UserCarryObjectsPtr=UserCarryObjectsPtr->prev;
		}
	}

	UserCarryObjectsPtr=UserCarryObjectsPtr->next;
}

if(found == FALSE) {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  	/* not found */
	return(-1);
}


SetLastError(currentuser,NO_ERROR);
return(0);
}

int GetRace(char *name,race *out) {
sqlite3_stmt *SQLStatementHandle;
char *nameout[BUF_SIZE];

ToUppercase(name,nameout);

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM RACES WHERE UPPER(NAME)=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	/* prepare SQL statement */
	return(-1);
}

/* bind parameters */
sqlite3_bind_text(SQLStatementHandle,1,nameout,strlen(nameout),NULL);

if(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {
	strncpy(out->name,sqlite3_column_text(SQLStatementHandle,RACE_NAME_COLUMN),BUF_SIZE);
	out->magic=sqlite3_column_int(SQLStatementHandle,RACE_MAGIC_COLUMN);
	out->strength=sqlite3_column_int(SQLStatementHandle,RACE_STRENGTH_COLUMN);
	out->agility=sqlite3_column_int(SQLStatementHandle,RACE_AGILITY_COLUMN);
	out->dexterity=sqlite3_column_int(SQLStatementHandle,RACE_DEXTERITY_COLUMN);
	out->luck=sqlite3_column_int(SQLStatementHandle,RACE_LUCK_COLUMN);
	out->wisdom=sqlite3_column_int(SQLStatementHandle,RACE_WISDOM_COLUMN);
	out->intelligence=sqlite3_column_int(SQLStatementHandle,RACE_INTELLIGENCE_COLUMN);
	out->stamina=sqlite3_column_int(SQLStatementHandle,RACE_STAMINA_COLUMN);

	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(0);
}

return(-1);
}

int GetClass(char *name,userclass *out) {
sqlite3_stmt *SQLStatementHandle;
char *nameout[BUF_SIZE];

ToUppercase(name,nameout);

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM CLASSES WHERE UPPER(NAME)=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	/* prepare SQL statement */
	return(-1);
}

sqlite3_bind_text(SQLStatementHandle,1,nameout,strlen(nameout),NULL);

if(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {
	strncpy(out->name,sqlite3_column_text(SQLStatementHandle,CLASS_NAME_COLUMN),BUF_SIZE);

	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(0);
}

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(-1);
}

int SetInventoryObjectOwnerInDatabase(char *owner,char *name) {
sqlite3_stmt *SQLStatementHandle;

/* prepare statement SQL */
if(sqlite3_prepare_v2(GetDatabaseHandle(),"UPDATE INVENTORY SET owner=? WHERE name=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) return(-1);

/* bind parameters */
sqlite3_bind_text(SQLStatementHandle,1,owner,strlen(owner),NULL);
sqlite3_bind_text(SQLStatementHandle,2,name,strlen(name),NULL);

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(0);
}

int CheckIfBanned(char *ipaddress) {
sqlite3_stmt *SQLStatementHandle;

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT IPADDRESS FROM BANS WHERE EXISTS IPAddress=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	/* prepare SQL statement */
	return(-1);
}

sqlite3_bind_text(SQLStatementHandle,1,ipaddress,strlen(ipaddress),NULL);		/* bind IP address to first parameter */

if(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {		/* is banned */
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(TRUE);
}

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(FALSE);
}

int RemoveFromInventoryInDatabase(user *currentuser,char *ObjectName,char *owner) {
sqlite3_stmt *SQLStatementHandle;

/* remove from database */

if(sqlite3_prepare_v2(GetDatabaseHandle(),"DELETE FROM INVENTORY WHERE NAME=? AND OWNER=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

sqlite3_bind_text(SQLStatementHandle,1,ObjectName,strlen(ObjectName),NULL);
sqlite3_bind_text(SQLStatementHandle,2,currentuser->username,strlen(currentuser->username),NULL);

if(sqlite3_step(SQLStatementHandle) == SQLITE_DONE) {		/* deleted OK */
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(0);
}

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);

SetLastError(currentuser,OBJECT_NOT_FOUND);
return(-1);
}

int CheckIfUserExists(char *username) {
sqlite3_stmt *SQLStatementHandle;

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT USERNAME FROM USERS WHERE USERNAME=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	return(-1);
}

sqlite3_bind_text(SQLStatementHandle,1,username,strlen(username),NULL);

if(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {		/* username exists */
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(TRUE);
}

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(FALSE);
}

int CheckIfRaceExists(char *name) {
sqlite3_stmt *SQLStatementHandle;

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT NAME FROM RACES WHERE NAME=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	return(-1);
}

sqlite3_bind_text(SQLStatementHandle,1,name,strlen(name),NULL);

if(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {		/* username exists */
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(TRUE);
}

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(FALSE);
}

int CheckIfClassExists(char *name) {
sqlite3_stmt *SQLStatementHandle;

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT NAME FROM CLASSES WHERE NAME=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	return(-1);
}

sqlite3_bind_text(SQLStatementHandle,1,name,strlen(name),NULL);

if(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {		/* username exists */
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(TRUE);
}

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(FALSE);
}

int DisconnectUser(user *currentuser,char *username) {
user *userptr;
bool UserFound=FALSE;

userptr=LoggedInUsers;

while(userptr != NULL) {
	if(regexp(userptr->username,username) == TRUE) {
		shutdown(userptr->socket,SHUT_RDWR);		/* shutdown socket */

		UserFound=TRUE;
	}
		
	userptr=userptr->next;
}

if(UserFound == FALSE) {		/* no user found */
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(0);
}

