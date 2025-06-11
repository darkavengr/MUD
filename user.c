#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define _XOPEN_SOURCE
#define _GNU_SOURCE

#ifdef __linux__
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdlib.h>
#endif

#ifdef _WIN32
#include "windows.h"
#include <winsock2.h>

#include <ws2tcpip.h>
// #define inet_ntop InetNtop
#endif

#include <crypt.h>

#include "bool.h"
#include "help.h"
#include "ban.h"
#include "class.h"
#include "race.h"
#include "errors.h"
#include "user.h"
#include "config.h"
#include "database.h"

user *users=NULL;
ban *bans=NULL;
ban *bans_last=NULL;
race *races=NULL;
class *classes=NULL;

/* titles */
char *MaleUserLevelNames[] = {"","Novice","Warrior","Hero","Champion","Superhero","Enchanter","Sorceror","Necromancer", \
			"Legend","Wizard","Arch Wizard","Dungeon Master" };

char *FemaleUserLevelNames[] = {"","Novice","Warrior","Heroine","Champion","Superheroine","Enchanteress","Sorceroress", \
			"Legend","Witch","Arch Witch","Dungeon Master" };

char *BanListPrompt="Press ENTER to see more bans or q to quit:";
char *BanConfigurationFile="config/ban.mud";
char *ClassConfigurationFile="config/classes.mud";
char *UserConfigurationFile="config/users.mud";
char *RaceConfigurationFile="config/races.mud";
char *ObjectConfigurationFile[BUF_SIZE];
int UserUpdated;

int BanUserByName(user *currentuser,char *username) {
user *usernext;

if(currentuser->status < WIZARD) {		/* not yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

usernext=users;		/* find user */

while(usernext != NULL) {
	if(regexp(usernext->name,username) == TRUE) {	/* found ip address */

		BanUserByIPAddress(currentuser,usernext->ipaddress);
		UpdateBanFile();
		return(0);
	}

	usernext=usernext->next;
}

SetLastError(currentuser,UNKNOWN_USER);
return(-1);
}

int BanUserByIPAddress(user *currentuser,char *ipaddress) {
ban *banlist;

if(currentuser->status < WIZARD) {		/* not yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

banlist=bans;

while(banlist != NULL) {
	if(*banlist->ipaddress && strcmp(banlist->ipaddress,ipaddress) == 0) {		/* ip address already banned */
		SetLastError(currentuser,ALREADY_BANNED);
		return(-1);
	}

	banlist=banlist->next;
}

if(bans == NULL) {
	bans=calloc(1,sizeof(ban));	/* add new link */
	if(bans == NULL) {		/* can't allocate */
		SetLastError(currentuser,NO_MEM);
		return(-1);
	}

	bans_last=bans;
}
else
{
	bans_last->next=calloc(1,sizeof(ban));	/* add new link */
	if(bans_last->next == NULL) {		/* can't allocate */
		SetLastError(currentuser,NO_MEM);
		return(-1);
	}

	bans_last=bans_last->next;
}

strcpy(bans_last->ipaddress,ipaddress);
bans_last->next=NULL;

return(0);
}

int UpdateBanFile(void) {
FILE *handle;
ban *bannext;

handle=fopen(BanConfigurationFile,"w");
if(handle == NULL) return(-1);		/* can't open */

bannext=bans;
while(bannext != NULL) {
	fprintf(handle,"%s\r\n",bannext->ipaddress);		/* write ip address */
	bannext=bannext->next;
}

fclose(handle);
return(0);
}

int ListBans(user *currentuser,char *banname) {
int count=0;
ban *bannext;
char *name[BUF_SIZE];
char *b;
char *buf[BUF_SIZE];

if(currentuser->status < WIZARD) {		/* not yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(!*banname) {		/* nop name */
	strcpy(name,"*");
}
else
{
	strcpy(name,banname);
}

bannext=bans;
while(bannext != NULL) {

	if(regexp(bannext->ipaddress,name) == TRUE) {	/* ban found */
		sprintf(buf,"%s\r\n",bannext->ipaddress);

		send(currentuser->handle,buf,strlen(buf),0);
	}
	
	bannext=bannext->next;
}

return(0);
}

int UnBanUserByIPAddress(user *currentuser,char *ipaddress) {
ban *next;
ban *last;

next=bans;

while(next != NULL) {
	last=next;

	if(*next->ipaddress && strcmp(next->ipaddress,ipaddress) == 0) {		/* ip address already banned */

		if(next == bans) {		/* first in list */
			bans=bans->next;
		}
		else if(next->next == NULL) {	/* last in list */
			last->next=NULL;
		}
		else
		{
			last->next=next->next;
		}

		free(next);
		return(0);
	}

	next=next->next;
}

SetLastError(currentuser,UNKNOWN_USER);
return(-1);
}

int LoadBans(void) {
ban *bannext;
char *LineBuffer[BUF_SIZE];
int LineCount;
FILE *handle;

bannext=bans;
LineCount=0;

handle=fopen(BanConfigurationFile,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",BanConfigurationFile);
	return(-1);
}

do {
	fgets(LineBuffer,BUF_SIZE,handle);		/* get and parse line */

	RemoveNewLine(LineBuffer);		/* remove newline character */

	if((char) *LineBuffer != '#')  {		/* skip comments */

		if(bans == NULL) {			/* first ban */
			bans=calloc(1,sizeof(ban));
			if(bans == NULL) {
				perror("\nmud:");
				exit(NOMEM);
			}

			bannext=bans;
		}
		else
		{
			bannext->next=calloc(1,sizeof(ban));
			bannext=bannext->next;

			if(bannext == NULL) {
				perror("\nmud:");
				exit(NOMEM);
			}

		}
	
		strcpy(bannext->ipaddress,LineBuffer);

		bannext->next=calloc(1,sizeof(ban));	/* add new link */
		if(bannext->next == NULL) break;

		bannext=bannext->next;
	}

} while(!feof(handle));

fclose(handle);
return(0);
}

int CheckIfBanned(char *name) {
ban *next;

next=bans;

while(next != NULL) { 
	if(regexp(next->ipaddress,name) == TRUE) return(TRUE);

	next=next->next;
}

return(FALSE);
}

	
/*
*force user to do something
*/

int ForceUser(user *currentuser,char *u,char *c) {
user *usernext;

if(currentuser->status < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,NOT_YET);
	return(0);
}

/*
* find user
*/

usernext=users;

while(usernext != NULL) {
	if(regexp(u,usernext->name) == TRUE && usernext->loggedin == TRUE) return(ExecuteCommand(usernext,c));    /* do command */ 
		
	usernext=usernext->next;
}

return(0);
}

/*
* give object to user
*/

int GiveObjectToUser(user *currentuser,char *u,char *o) {
user *usernext;
roomobject *RoomObjectPtr;
roomobject *ourobjectlast;
roomobject *temp;
int found=0;
int  ObjectFound=0;
roomobject *ourobject;
char *buf[BUF_SIZE];

/*
* find user
*/

usernext=users;

while(usernext != NULL) {
	if(regexp(usernext->name,u) == TRUE) {		/* found object */
		found=TRUE;
		break;
	}

	usernext=usernext->next;
}

if(found == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

/*
* find object
*/

ourobject=currentuser->carryobjects;
ourobjectlast=ourobject;

while(ourobject != NULL) {
	if(regexp(ourobject->name,o) == TRUE) {		/* found object */

		RoomObjectPtr=usernext->carryobjects;
	
		if(RoomObjectPtr != NULL) {				/* find end */				
			while(RoomObjectPtr->next != NULL) RoomObjectPtr=RoomObjectPtr->next; 

			RoomObjectPtr->next=calloc(1,sizeof(roomobject));
			if(RoomObjectPtr->next == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);	
			}

			RoomObjectPtr=RoomObjectPtr->next;
		}
		else
		{						
			usernext->carryobjects=calloc(1,sizeof(roomobject));	/* allocate objects */ 
			RoomObjectPtr=usernext->carryobjects;

			if(RoomObjectPtr == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}
		}    


		memcpy(RoomObjectPtr,ourobject,sizeof(roomobject));	/* copy data */
	
		if(ourobject == currentuser->carryobjects) {		/* first object */
			ourobject=ourobject->next;

			free(currentuser->carryobjects);   
			currentuser->carryobjects=ourobject;
		}

		if(ourobject->next == NULL) {		/* last object */
			free(ourobject);	
		}

		if(ourobject != currentuser->carryobjects && ourobject->next != NULL) {      
			ourobjectlast->next=ourobject->next;	/* skip over over object */
			free(ourobject);
		}

		ObjectFound=TRUE;
	}

	ourobjectlast=ourobject;
	ourobject=ourobject->next;
}

if(ObjectFound == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER); /* no object found */
	return(-1);
}

return(0);
}

/*
* display inventory
*/

int DisplayInventory(user *currentuser,char *username) {
char *whichuser[BUF_SIZE];
char *buf[BUF_SIZE];
roomobject *roomnext;
user *usernext;
roomobject *RoomObjectPtr;

if(!*username) {
	strcpy(whichuser,currentuser->name);   /* use default user */

}
else
{

	if(currentuser->status < WIZARD) {		/* can't do this yet */
		SetLastError(currentuser,NOT_YET);
		return(0);
	}

	strcpy(whichuser,username);
}

usernext=users;

while(usernext != NULL) {
	if(regexp(usernext->name,whichuser) == TRUE && usernext->loggedin == TRUE) {	/* found user */

		if(usernext->carryobjects == NULL) {                   /* not carrying anything */
			sprintf(buf,"%s is carrying nothing\r\n",usernext->name);

			usernext=usernext->next;
			send(currentuser->handle,buf,strlen(buf),0);
			continue;
		}

		sprintf(buf,"%s is carrying: ",usernext->name);
		send(currentuser->handle,buf,strlen(buf),0);

		RoomObjectPtr=usernext->carryobjects;

		while(RoomObjectPtr != NULL) {
			send(currentuser->handle,RoomObjectPtr->name,strlen(RoomObjectPtr->name),0);	/* display objects in inventory */
			send(currentuser->handle," ",1,0);
	
			RoomObjectPtr=RoomObjectPtr->next;
		}
	
		send(currentuser->handle,"\r\n",2,0);
	}

	usernext=usernext->next;
}

return(0);
}


/*
* kill user
*/
int KillUser(user *currentuser,char *username) {
room *roomnext;
user *usernext;
monster *monsternext;
char *OutputMessage[BUF_SIZE];
int found=FALSE;
int count;
room *currentroom;
char *UserInventoryFile[BUF_SIZE];

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

usernext=users;
while(usernext != NULL) {
	if(regexp(usernext->name,username) == TRUE && usernext->loggedin == TRUE) {		/* found user */
		found=TRUE;

		if(currentuser->status < usernext->status ) {  /* wizards can't be killed */
			SetLastError(currentuser,KILL_WIZARD);
			return(-1);
		}

		if(usernext->gender == MALE) {
			sprintf(OutputMessage,"You were given the finger of death by %s the %s\r\n",currentuser->name,MaleUserLevelNames[currentuser->status]);
		}
		else
		{
			sprintf(OutputMessage,"You were given the finger of death by %s the %s\r\n",currentuser->name,FemaleUserLevelNames[currentuser->status]);
		}

		send(usernext->handle,OutputMessage,strlen(OutputMessage),0);
		close(usernext->handle);

		usernext->next=usernext->last;
		free(usernext);

		UpdateUser(usernext,username,"",0,0,"",0,0,0,0,"","",0);          /* remove user */

		sprintf(UserInventoryFile,"%config/%s.inv",usernext->name);		/* get absolute path of user inventory */
		unlink(UserInventoryFile);		/* delete inventory file */

		return(-1);		
	}

	usernext=usernext->next;
}

/*
* if monster
*/

found=FALSE;

for(count=0;count<currentroom->monstercount;count++) {
	if(regexp(username,currentroom->roommonsters[count].name) == TRUE) {		/* found monster */
		DeleteMonster(currentroom->room,count);
		found=TRUE;
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
user *usernext;
char *OutputMessage[BUF_SIZE];

usernext=users;

while(usernext != NULL) {

	if(usernext->room == currentuser->room) {	/* in same room */
		sprintf(OutputMessage,"*%s %s\r\n",currentuser->name,message);

		SendMessageToAllInRoom(usernext->room,OutputMessage);	/* send message */
	}

	usernext=usernext->next;
}

return(0);
}

/*
* disconnect user
*/

void quit(user *currentuser) {
char *buf[BUF_SIZE];

sprintf(buf,"%s has disconnected\r\n",currentuser->name);

SendMessageToAllInRoom(currentuser->room,buf);

currentuser->loggedin=FALSE; /* mark as logged out */

DisconnectUser(currentuser);		/* disconnect user */
}

int DisplayScore(user *currentuser,char *username) {
user *usernext;
char *buf[BUF_SIZE];
char *name[BUF_SIZE];
int found;
void *titleptr;

found=FALSE;

if(!*username) {			/* find score for current user */
	strcpy(name,currentuser->name);
}
else
{
	if(currentuser->status < WIZARD) {		/* not yet */
		SetLastError(currentuser,NOT_YET);
		return(-1);
	}

	strcpy(name,username);
}

if(currentuser->gender == MALE) {		/* which user title */
	titleptr=MaleUserLevelNames[currentuser->status];
}
else
{
	titleptr=FemaleUserLevelNames[currentuser->status];
}

usernext=users;

while(usernext != NULL) {
	if(regexp(usernext->name,name) == TRUE) {		/* found user */

		sprintf(buf,"Magic Points:%d\r\nStamina Points:%d\r\nExperience Points:%d\r\nLevel: %s (%d)\r\n", \
										usernext->magicpoints,\
											usernext->staminapoints,\
										usernext->experiencepoints,\
										titleptr,\
										usernext->status);

		send(currentuser->handle,buf,strlen(buf),0);
		found=TRUE;
	}

	usernext=usernext->next;
}

if(found == FALSE)  {
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(0);
}

/*
* send private message to someone
*/

int SendMessageToAllInRoom(int room,char *message) {
user *usernext;

usernext=users;

while(usernext != NULL) {
	if(usernext->room == room && usernext->loggedin == TRUE) send(usernext->handle,message,strlen(message),0);	/* found user */

	usernext=usernext->next;
}

return(0);
}

/*
* send private message to someone
*/

int SendMessage(user *currentuser,char *username,char *message) {
int count=0;
char *buf[BUF_SIZE];
user *usernext;

usernext=users;

while(usernext != NULL) {
	if(regexp(username,usernext->name) == TRUE) {		/* found user */

		if(currentuser->flags & USER_INVISIBLE) {
			count++;
			sprintf(buf,"Somebody whispers, %s\r\n",username,message);
		}
		else
		{
			count++;
			sprintf(buf,"[%s] %s\r\n",username,message);
		}

		send(currentuser->handle,buf,strlen(buf),0);
	}

	usernext=usernext->next;
}

if(count > 0) {			/* unknown user */
	SetLastError(currentuser,UNKNOWN_USER);
	return(-1);
}

return(0);
}

/*
* take object from user
*/

int TakeObject(user *currentuser,char *username,char *object) {
user *usernext;
roomobject *RoomObjectPtr;
roomobject *RoomObjectLast;
roomobject *myobj;
int found=0;
int  ObjectFound=0;
char *buf[BUF_SIZE];

if(currentuser->status < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/*
* find user
*/

usernext=users;

while(usernext != NULL) {
	if(regexp(usernext->name,username) == TRUE) {		/* found user */
		found=TRUE;
		break;
	}

	usernext=usernext->next;
}

if(found == FALSE) {
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/*
* find object
*/

RoomObjectPtr=usernext->carryobjects;

while(RoomObjectPtr != NULL) {

	RoomObjectLast=RoomObjectPtr;

	if(regexp(RoomObjectPtr->name,object) == TRUE) {		/* found object */

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
		
			currentuser->carryobjects=currentuser->carryobjects_last->next;
		}    
	
		memcpy(currentuser->carryobjects_last,RoomObjectPtr,sizeof(roomobject));	/* copy data */
	
		if(RoomObjectPtr == usernext->carryobjects) {		/* first object */
			RoomObjectPtr=RoomObjectPtr->next;
	
			free(usernext->carryobjects);   
			usernext->carryobjects=RoomObjectPtr;
		}
		else if(RoomObjectPtr != usernext->carryobjects && RoomObjectPtr->next != NULL) {      
			RoomObjectLast->next=RoomObjectPtr->next;	/* skip over over object */        
			free(RoomObjectPtr);
		}
		else if(RoomObjectPtr == usernext->carryobjects && RoomObjectPtr->next != NULL) {		/* last object */          
			 free(RoomObjectPtr);	
		}
	
		ObjectFound=TRUE;
	}

	RoomObjectPtr=RoomObjectPtr->next;
}

if(ObjectFound == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER); /* no object found */
	return(-1);
}

return(0);
}

/*
* update user info
*/

int UpdateUser(user *currentuser,char *username,char *password,int homeroom,int userlevel,char *description,int magicpoints,int staminapoints,int experiencepoints,int gender,char *racex,char *classx,int flags) {
int dead=0;
int count;
char *tokens[BUF_SIZE][BUF_SIZE];
user *usernext;
roomobject *RoomObjectPtr;
char *OutputMessage[BUF_SIZE];
int newlevel;
race *racenext;
class *classnext;
CONFIG config;

if(homeroom < 0) homeroom=0;			/* sanity check */
if(userlevel < 0) userlevel=0;
if(magicpoints < 0) magicpoints=0;
if(staminapoints < 0) staminapoints=0;
if(experiencepoints < 0) experiencepoints=0;

GetConfigurationInformation(&config);

usernext=users;
while(usernext != NULL) {

/*
* if the new entry value is 0 then it is ignored and the value is unchanged,
if the stamina points are 0 the user is killed and will not be included in the updated file
*/

	if(regexp(usernext->name,username) == TRUE) {			/* found user */

		strcpy(usernext->name,username);
		if(*password) strcpy(usernext->password,password);
	
		if(homeroom > 0) usernext->homeroom=homeroom;
		if(userlevel > 0) usernext->status=userlevel;
		if(magicpoints > 0) usernext->magicpoints=magicpoints;

		if(flags > 0) usernext->flags=flags;

		usernext->staminapoints=staminapoints;

		/*
		* the user is dead, long live the user
		*/

		if(usernext->staminapoints <= 0 && (usernext->status < WIZARD)) {

			usernext->loggedin=FALSE;
			usernext->staminapoints=DEFAULT_STAMINAPOINTS;		/* reset user */
			usernext->magicpoints=DEFAULT_MAGICPOINTS;
			usernext->experiencepoints=0;
			usernext->status=NOVICE;
			usernext->homeroom=1;

			sprintf(OutputMessage,"%s is dead\n",usernext->name);
			SendMessageToAllInRoom(usernext->room,OutputMessage);

			DropObject(usernext,"*"); 		/* drop objects carried by user */

			close(usernext->handle);   
			return(0);
		}

		/* adjust new level */

		 if(experiencepoints > 0) {
			if(experiencepoints < config.pointsforwarrior) newlevel=NOVICE;
			if((experiencepoints >= config.pointsforwarrior) && (experiencepoints < config.pointsforhero)) newlevel=WARRIOR;
			if((experiencepoints >= config.pointsforhero) && (experiencepoints < config.pointsforchampion)) newlevel=HERO;
			if((experiencepoints >= config.pointsforchampion) && (experiencepoints < config.pointsforsuperhero)) newlevel=CHAMPION;
			if((experiencepoints >= config.pointsforsuperhero) && (experiencepoints < config.pointsforenchanter)) newlevel=SUPERHERO;
			if((experiencepoints >= config.pointsforenchanter) && (experiencepoints < config.pointsforsorceror)) newlevel=ENCHANTER;
			if((experiencepoints >= config.pointsforsorceror) && (experiencepoints < config.pointsfornecromancer)) newlevel=SORCEROR;
			if((experiencepoints >= config.pointsfornecromancer) && (experiencepoints < config.pointsforlegend)) newlevel=NECROMANCER;
			if((experiencepoints >= config.pointsforlegend) && (experiencepoints < config.pointsforwizard)) newlevel=LEGEND;
			if((experiencepoints >= config.pointsforwizard)) newlevel=WIZARD;

			if(newlevel > usernext->status || newlevel < usernext->status) {		/* new level */
				usernext->status=newlevel;
	
				if(usernext->gender == MALE) {
					sprintf(OutputMessage,"You are now a %s!\n",MaleUserLevelNames[newlevel]);
				}
				else
				{
					sprintf(OutputMessage,"You are now a %s!\n",FemaleUserLevelNames[newlevel]);
				}
			}

			send(usernext->handle,OutputMessage,strlen(OutputMessage),0);
		}
		

		usernext->experiencepoints=experiencepoints;

		if(gender > 0) usernext->gender=gender;

		if(*racex) {
			racenext=races;				/* find race */

			while(racenext != NULL) {
				if(strcmp(racenext->name,racex) == 0) {		/* found race */
					usernext->race=racenext;
					break;
				}

				racenext=racenext->next;
			}
		}

		if(*classx) {
			classnext=classes;				/* find class */

			while(classnext != NULL) {
				if(strcmp(classnext->name,classx) == 0) {		/* found class */
					usernext->userclass=classnext;
					break;
				}
	
				classnext=classnext->next;
			}
		}

		if(*description) strcpy(usernext->desc,description);

		UserUpdated=TRUE;
	}

	usernext=usernext->next;
}

SetDatabaseUpdateFlag();			/* set database updated flag */
return(0);
}

/*
* update users file
*/

int UpdateUsersFile(void) {
FILE *handle;
FILE *handleinv;
user *usernext;
roomobject *RoomObjectPtr;
race *racenext;
class *classnext;
char *InventoryFile[BUF_SIZE];
char *GenderString[BUF_SIZE];

handle=fopen(UserConfigurationFile,"w");
if(handle == NULL) return(-1);

usernext=users;

while(usernext != NULL) {

	racenext=usernext->race;				/* find race */
	classnext=usernext->userclass;

	if(usernext->gender == MALE) {		/* gender */
		strcpy(GenderString,"male");
	}
	else
	{
		strcpy(GenderString,"female");
	}

	fprintf(handle,"%s:%s:%d:%d:%s:%d:%d:%d:%s:%s:%s:%d\n",usernext->name,usernext->password,usernext->homeroom,usernext->status,\
						usernext->desc,usernext->magicpoints,usernext->staminapoints,usernext->experiencepoints, \
						GenderString,racenext->name,classnext->name,usernext->flags);

	/* update inventory file */

	sprintf(InventoryFile,"/config/%s.inv",usernext->name);			/* get path */

	handleinv=fopen(InventoryFile,"w");
	if(handleinv != NULL) {		/* can't open */
		RoomObjectPtr=usernext->carryobjects;

		while(RoomObjectPtr != NULL) {
			fprintf(handleinv,"%s:%d:%d:%d:%d:%s\n",RoomObjectPtr->name,RoomObjectPtr->staminapoints,RoomObjectPtr->magicpoints,\

			RoomObjectPtr->attackpoints,RoomObjectPtr->generateprob,RoomObjectPtr->desc);			

			RoomObjectPtr=RoomObjectPtr->next;
		}

		fclose(handleinv);
	}
		
	usernext=usernext->next;
}

fclose(handle);
return(0);
}

/*
* set user points (magic/stamina/experience)
*/

int SetUserPoints(user *currentuser,char *username,char *amountstr,int which) {
user *usernext;
int amount;

usernext=users;

while(usernext != NULL) {

	if(regexp(usernext->name,username) == TRUE) {	/* if user found */

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
			return(UpdateUser(currentuser,usernext->name,"",0,0,"",usernext->magicpoints+amount,0,0,0,"","",0));
		}
		else if(which == STAMINAPOINTS) {
			return(UpdateUser(currentuser,usernext->name,"",0,0,"",0,usernext->staminapoints+amount,0,0,"","",0));
		}
		else if(which == EXPERIENCEPOINTS) {
			return(UpdateUser(currentuser,usernext->name,"",0,0,"",0,0,usernext->experiencepoints+amount,0,"","",0));
		}
	}

	usernext=usernext->next;
}

SetLastError(currentuser,UNKNOWN_USER);		/* user not found */
return(-1);
}

/*
* set user level */

int SetUserLevel(user *currentuser,char *username,char *levelstr) {
user *usernext;
int level;

if(currentuser->status < WIZARD) {     /* not wizard */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

usernext=users;
while(usernext != NULL) {

	if(regexp(usernext->name,username) == TRUE) {		/* found user */
	
		if((char) *levelstr == '+') {			/* add points */
			sscanf(levelstr,"+%d",&level);

			if(usernext->status+level > 12) {
				SetLastError(currentuser,INVALID_LEVEL);
				return(-1);
			}
	
			 if(usernext->status+level > currentuser->status) {		/* can't set level above own level */
				SetLastError(currentuser,NOT_YET);
				return(-1);
			 }

			UpdateUser(currentuser,username,"",0,usernext->status+level,"",0,0,0,0,"","",0);   /* set level */
			return(0);
		}
		else if((char) *levelstr == '-') {			/* subtract points */
			sscanf(levelstr,"-%d",&level);

			if((usernext->status-level <= 0) || (usernext->status+level <= 0)) {
				SetLastError(currentuser,INVALID_LEVEL);
				return(-1);
			}
	
			UpdateUser(currentuser,username,"",0,usernext->status-level,"",0,0,0,0,"","",0);   /* set level */
			return(0);
		}
		else {
			level=atoi(levelstr);

			if((level > 12) || (level <= 0)) {
				SetLastError(currentuser,INVALID_LEVEL);
				return(-1);
			}

			UpdateUser(currentuser,username,"",0,level,"",0,0,0,0,"","",0);   /* set level */
			return(0);
		}
	}

	usernext=usernext->next;
}

return(0);
}


/* set gender */
int SetUserGender(user *currentuser,char *u,char *gender) {

if(currentuser->status < WIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(strcmp(gender,"male") == 0) {
	return(UpdateUser(currentuser,u,"",0,0,"",0,0,0,MALE,"","",0));
}

if(strcmp(gender,"female") == 0) {
	return(UpdateUser(currentuser,u,"",0,0,"",0,0,0,FEMALE,"","",0));
}

SetLastError(currentuser,BAD_GENDER);
return(-1);
}

int LoadRaces(void) {
race *racenext;
FILE *handle;
int LineCount;
char *RaceTokens[BUF_SIZE][BUF_SIZE];
char *LineBuffer[BUF_SIZE];
int ErrorCount=0;

racenext=races;
LineCount=0;

handle=fopen(RaceConfigurationFile,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",RaceConfigurationFile);
	exit(NOCONFIGFILE);
}

while(!feof(handle)) {
	fgets(LineBuffer,BUF_SIZE,handle);		/* get and parse line */

	if((char) *LineBuffer == '#')  continue;		/* skip comments */
	if((char) *LineBuffer == '\n')  continue;		/* skip newline */

	RemoveNewLine(LineBuffer);		/* remove newline character */

	LineCount++;

	TokenizeLine(LineBuffer,RaceTokens,":\n");				/* tokenize line */

	if(strcmp(RaceTokens[0],"begin_race") == 0) {	/* end */

		if(races == NULL) {			/* first race */
			races=calloc(1,sizeof(race));
			if(races == NULL) {
				perror("\nmud:");
				exit(NOMEM);
			}

			racenext=races;
		}
		else
		{
			racenext->next=calloc(1,sizeof(race));
			racenext=racenext->next;

			if(racenext == NULL) {
				perror("\nmud:");
				exit(NOMEM);
			}

		}

		strcpy(racenext->name,RaceTokens[1]);
		continue;			
	}

	if(strcmp(RaceTokens[0],"intelligence") == 0) {	/* race points used */
		racenext->intelligence=atoi(RaceTokens[1]);
		continue;			
	}

	if(strcmp(RaceTokens[0],"strength") == 0) {
		racenext->strength=atoi(RaceTokens[1]);
		continue;			
	}

	if(strcmp(RaceTokens[0],"wisdom") == 0) {
		racenext->wisdom=atoi(RaceTokens[1]);
		continue;			
	}  

	if(strcmp(RaceTokens[0],"dexterity") == 0) {
		racenext->dexterity=atoi(RaceTokens[1]);
		continue;			
	}

	if(strcmp(RaceTokens[0],"luck") == 0) {
		racenext->luck=atoi(RaceTokens[1]);
		continue;			
	}

	if(strcmp(RaceTokens[0],"magic") == 0) {
		racenext->magic=atoi(RaceTokens[1]);
		continue;			
	}  

	if(strcmp(RaceTokens[0],"agility") == 0) {
		racenext->agility=atoi(RaceTokens[1]);
		continue;			
	}

	if(strcmp(RaceTokens[0],"stamina") == 0) {
		racenext->stamina=atoi(RaceTokens[1]);
		continue;			
	}

	if(strcmp(RaceTokens[0],"end") == 0) continue;

	printf("\nmud: %d: uknown configuration option %s in %s\n",LineCount,RaceTokens[0],RaceConfigurationFile);		/* unknown configuration option */
	ErrorCount++;
}


fclose(handle);
return(ErrorCount);
}


int LoadClasses(void) {
class *classnext;
FILE *handle;
int LineCount;
char *RaceTokens[10][BUF_SIZE];
char *LineBuffer[BUF_SIZE];
int ErrorCount=0;

classnext=classes;
LineCount=0;

handle=fopen(ClassConfigurationFile,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",ClassConfigurationFile);

	exit(NOCONFIGFILE);
}

while(!feof(handle)) {
	fgets(LineBuffer,BUF_SIZE,handle);		/* get and parse line */

	RemoveNewLine(LineBuffer);		/* remove newline character */

	LineCount++;

	if(classes == NULL) {			/* first class */
		classes=calloc(1,sizeof(class));
		if(classes == NULL) {
			perror("\nmud:");
			exit(NOMEM);
		}

		classnext=classes;
	}
	else
	{
		classnext->next=calloc(1,sizeof(class));
		classnext=classnext->next;

		if(classnext == NULL) {
			perror("\nmud:");
			exit(NOMEM);
		}
	}


	strcpy(classnext->name,LineBuffer);
}

fclose(handle);
return(ErrorCount);
}

int LoadUsers(void) {
user *usernext;
FILE *handle;
int LineCount;
char *RaceTokens[100][BUF_SIZE];
char *LineBuffer[BUF_SIZE];
int ErrorCount=0;
class *userclass;
class *classlast;
race *racelast;
race *userrace;

handle=fopen(UserConfigurationFile,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",UserConfigurationFile);
	exit(NOCONFIGFILE);
}

LineCount=0;

while(!feof(handle)) {
	fgets(LineBuffer,BUF_SIZE,handle);

	if(strlen(LineBuffer) < 2) continue;		/* ignore empty lines */

	if(feof(handle)) break;		/* at end */

	RemoveNewLine(LineBuffer);		/* remove newline character */

	TokenizeLine(LineBuffer,RaceTokens,":\n");				/* tokenize line */

	if(users == NULL) {			/* first user */
		users=calloc(1,sizeof(user));
		if(users == NULL) {
			perror("\nmud:");
			exit(NOMEM);
		}

		usernext=users;
	}
	else
	{
		usernext->next=calloc(1,sizeof(user));
		usernext=usernext->next;

		if(usernext == NULL) {
			perror("\nmud:");
			exit(NOMEM);
		}

	}

	strcpy(usernext->name,RaceTokens[USERNAME]);		/* get details */
	strcpy(usernext->password,RaceTokens[PASSWORD]);		/* get details */
	usernext->homeroom=atoi(RaceTokens[HOMEROOM]);
	usernext->status=atoi(RaceTokens[USERLEVEL]);
	strcpy(usernext->desc,RaceTokens[DESCRIPTION]);
	usernext->magicpoints=atoi(RaceTokens[MAGICPOINTS]);
	usernext->staminapoints=atoi(RaceTokens[STAMINAPOINTS]);
	usernext->experiencepoints=atoi(RaceTokens[EXPERIENCEPOINTS]);
	usernext->gender=atoi(RaceTokens[GENDER]);
	usernext->handle=0;
	usernext->flags=atoi(RaceTokens[USERFLAGS]);
	usernext->next=NULL;

	userrace=races;		/* load race */
	racelast=races;

	while(userrace != NULL) {
		if(strcmp(userrace->name,RaceTokens[RACE]) == 0) {		/* FOund race */
			usernext->race=racelast;
			break;
		}
	}
	racelast=userrace;
	userrace=userrace->next;
}

/* find user class from class name */

userclass=classes;		/* load class */
classlast=classes;

while(userclass != NULL) {
	if(strcmp(userclass->name,RaceTokens[CLASS]) == 0) {		/* FOund class */
		usernext->userclass=classlast;
		break;
	}

	classlast=userclass;
	userclass=userclass->next;
}

/* update inventory file */

fclose(handle);
return(ErrorCount);
}

int SetVisibleMode(user *currentuser,char *name,int mode) {
user *next=users;

if(currentuser->status < WIZARD) {     /* not wizard */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

while(next != NULL) {
	if(strcmp(next->name,name) == 0) {

		if(mode == 0) {			/* go visible */
			next->flags &= USER_INVISIBLE;
		}
		else
		{
			next->flags |= USER_INVISIBLE;
		}

		return(0);
	}

	next=next->next;
}

SetLastError(currentuser,UNKNOWN_USER);
return(-1);
}

int GagUser(user *currentuser,char *name,int mode) {
user *next=users;

if(currentuser->status < WIZARD) {     /* not wizard */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(*name) {
	next=currentuser;
}
else
{
	while(next != NULL) {
		if(strcmp(next->name,name) == 0) break;

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

int wall(user *currentuser,char *m) {
user *usernext;
char *buf[BUF_SIZE];

if(currentuser->status < WIZARD) {		/* only wizard or higher users can send global message */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

usernext=users;

while(usernext != NULL) {
	
	if(usernext->loggedin == TRUE) {
		sprintf(buf,"[GLOBAL MESSAGE] %s\n",m);

		send(usernext->handle,buf,strlen(buf),0);			/* send message to every user */
	}

	usernext=usernext->next;
}

return(0);
}

/*
* see who's online
*/

int who(user *currentuser,char *username) {
char *OutputMessage[BUF_SIZE];
char *NameBuffer[BUF_SIZE];
int found=FALSE;
user *usernext;

if(!*username) {
	strcpy(NameBuffer,"*");          /* all users if no username */
}
else
{
	strcpy(NameBuffer,username);          
}

/*
* show users
*/

usernext=users;

while(usernext != NULL) {
	if((regexp(usernext->name,NameBuffer) == TRUE) && (usernext->loggedin == TRUE)  && ((usernext->flags & USER_INVISIBLE) == 0)) {			/* found user */
		if(usernext->gender == MALE) {
			sprintf(OutputMessage,"%s the %s is in %s (#%d)\r\n",usernext->name,MaleUserLevelNames[usernext->status],usernext->roomname,usernext->room);
		}
		else
		{
			sprintf(OutputMessage,"%s the %s is in %s (#%d)\r\n",usernext->name,FemaleUserLevelNames[usernext->status],usernext->roomname,usernext->room);
		}

		send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);
		found=TRUE;  
	}

	usernext=usernext->next;
}

if(found == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER);		/* unknown user */
	return(-1);
}

return(0);
}


int go(user *currentuser,int RoomNumber) {
room *roomnext;
char *buf[BUF_SIZE];

if(RoomNumber == 0) {		/* invalid room */
	SetLastError(currentuser,BAD_DIRECTION);
	return(-1);
}

if(currentuser->room != RoomNumber) {	/* send leaving message */
	sprintf(buf,"%s has left\r\n",currentuser->name);
	SendMessageToAllInRoom(currentuser->room,buf);
}

if(GetRoomFlags(RoomNumber) & ROOM_PRIVATE) {
	SetLastError(currentuser,BAD_DIRECTION);
	return(-1);
}	

strcpy(currentuser->roomname,GetRoomName(RoomNumber));

currentuser->room=RoomNumber;
currentuser->roomptr=GetRoomPointer(RoomNumber); 		/* save pointer to current room */

sprintf(buf,"%s has entered\r\n",currentuser->name);
SendMessageToAllInRoom(currentuser->room,buf);

look(currentuser,"");		/* look at new room */

if(GetRoomFlags(RoomNumber) & ROOM_DEAD) {
	KillUser(currentuser,currentuser->name);
	return(-1);
}	

return(0);
}

/*
* move object or player
*/

int MoveObject(user *currentuser,char *ObjectName,int RoomNumber) {
roomobject *RoomObjectPtr;
roomobject *DestinationObject;
int destination;
user *usernext;
room *currentroom;
room *DestinationRoom;
int FoundRoom=FALSE;
int found=FALSE;
CONFIG config;

GetConfigurationInformation(&config);

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {      /* not wizard */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(RoomNumber > GetNumberOfRooms()) {			/* can't find room */
	SetLastError(currentuser,BAD_ROOM);
	return(-1);
}

DestinationRoom=GetRoomPointer(RoomNumber);		/* point to room */

/* move object */
	
RoomObjectPtr=currentroom->roomobjects;

while(RoomObjectPtr != NULL) {
	if(regexp(RoomObjectPtr->name,ObjectName) == 0 ) {				/* if object matches */
		
		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(currentroom->owner,currentuser->name) == 0) && (currentroom->attr & OBJECT_MOVEABLE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			if((strcmp(currentroom->owner,currentuser->name) == 0) && (currentroom->attr & OBJECT_MOVEABLE_OWNER) == 0) {
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


/*
* move player
*/

/* find user */

usernext=users;

while(usernext != NULL) {
	if(regexp(usernext->name,ObjectName) == 0 && usernext->loggedin == TRUE) {       /* if object matches */

		if(currentuser->status < usernext->status) {  /* can't move user unless wizard or higher level */
			SetLastError(currentuser,NOT_YET);
			return(-1);
		}

		if(go(usernext->handle,RoomNumber) == -1) return(-1);

		found=TRUE;
	}

	usernext=usernext->next;
}

if(found == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER); /* unknown object */
	return(-1);
}

return(0);
}

int getuser(char *name,user *buf) {
user *usernext;

usernext=users;

while(usernext != NULL) {
	if(regexp(usernext->name,name) == 0 && usernext->loggedin == TRUE) {       /* if object matches */
		memcpy(buf,usernext,sizeof(user));
		return(0);
	}

	usernext=usernext->next;
}

return(-1);
}

int LoginUser(int messagesocket,char *username,char *password) {
char *encryptedpassword[BUF_SIZE];
char *RaceTokens[BUF_SIZE][BUF_SIZE];
FILE *handle;
char *buf[BUF_SIZE];
user *usernext;
user *userlast;
roomobject *RoomObjectPtr;
roomobject *RoomObjectLast;
int count;
struct sockaddr_in clientip;
socklen_t clientiplen;
char *ipaddress[BUF_SIZE];
char *UserInventoryFile[BUF_SIZE];

clientiplen=sizeof(struct sockaddr_in);			/* get ip address */
getpeername(messagesocket,(struct sockaddr*)&clientip,&clientiplen);

strcpy(ipaddress,inet_ntoa(clientip.sin_addr));

strcpy(encryptedpassword,crypt(password,username));

usernext=users;
userlast=users;

while(usernext != NULL) {
/* check username and password */
	if(strcmp(username,usernext->name) == 0 && strcmp(encryptedpassword,usernext->password) == 0) {
		strcpy(usernext->ipaddress,ipaddress);	/* get ip address */

		usernext->loggedin=TRUE;		/* user logged in */
		usernext->handle=messagesocket;		/* tcp socket */
		usernext->room=usernext->homeroom;	/* room */
		usernext->roomptr=GetRoomPointer(usernext->homeroom);

		/*
		* load user inventory
		*/

		sprintf(UserInventoryFile,"config/%s.inv",usernext->name);		/* get absolute path of user inventory */

		usernext->carryobjects=calloc(1,sizeof( roomobject));		/* allocate objects */
		if(usernext->carryobjects == NULL) {
			PrintError(messagesocket,NO_MEM);
			return(-1);		/* can't allocate */
		}

		RoomObjectPtr=usernext->carryobjects;

		handle=fopen(UserInventoryFile,"rb");
		if(handle != NULL) {
			while(!feof(handle)) {
				fgets(buf,BUF_SIZE,handle);	
				if(feof(handle)) break;
	
				RemoveNewLine(buf);		/* remove newline character */

				TokenizeLine(buf,RaceTokens,":");		/* tokenize line */
				strcpy(RoomObjectPtr->name,RaceTokens[OBJECT_NAME]);
				RoomObjectPtr->staminapoints=atoi(RaceTokens[OBJECT_STAMINAPOINTS]);
				RoomObjectPtr->magicpoints=atoi(RaceTokens[OBJECT_MAGICPOINTS]);
				RoomObjectPtr->attr=atoi(RaceTokens[OBJECT_ATTR]);
				RoomObjectPtr->attackpoints=atoi(RaceTokens[OBJECT_ATTACKPOINTS]);
				RoomObjectPtr->generateprob=atoi(RaceTokens[OBJECT_GENERATEPROB]);
				strcpy(RoomObjectPtr->desc,RaceTokens[OBJECT_DESCRIPTION]);
				strcpy(RoomObjectPtr->owner,RaceTokens[OBJECT_OWNER]);

				RoomObjectPtr->next=calloc(1,sizeof( roomobject));		/* allocate objects */
				if(usernext->carryobjects == NULL) {
					PrintError(messagesocket,NO_MEM);
					return(-1);		/* can't allocate */
				}

				RoomObjectPtr=RoomObjectPtr->next;
			}
		
			RoomObjectPtr->next=NULL;

			fclose(handle);

			if(go(usernext,usernext->homeroom) == -1) return(-1);
		}

		return(0);
	}


	usernext=usernext->next;
}

return(-1);
}

int CreateUser(int socket,char *name,char *pass,int gender,char *description,char *racex,char *classx) {
user *usernext;
user *userlast;
struct sockaddr_in clientip;
socklen_t clientiplen;
char *ipaddress[BUF_SIZE];
race *racenext;
class *classnext;
race *racelast;
class *classlast;

if(usernext == NULL) {
	users=calloc(1,sizeof(user));		/* add to end */
	usernext=users;
	userlast=users;
}
else
{
	userlast=usernext;
	usernext=users;

	while(usernext != NULL) {
		userlast=usernext;
		usernext=usernext->next;
	}

	userlast->next=calloc(1,sizeof(user));		/* add to end */
	if(userlast->next == NULL) return(-1);
}

usernext=userlast->next;

strcpy(usernext->name,name);
strcpy(usernext->password,pass);
strcpy(usernext->desc,description);
usernext->status=NOVICE;
usernext->homeroom=1;
usernext->room=1;
usernext->gender=gender;
usernext->magicpoints=DEFAULT_MAGICPOINTS;
usernext->staminapoints=DEFAULT_STAMINAPOINTS;
usernext->experiencepoints=0;
usernext->loggedin=TRUE;
usernext->handle=socket;
usernext->flags=0;
usernext->roomptr=GetRoomPointer(1);

racenext=races;			/* find race */
racelast=races;

while(racenext != NULL) {
	if(strcmp(racenext->name,racex) == 0) {		/* found race */
		usernext->race=racelast->next;
		break;
	}

	racenext=racenext->next;
}

classnext=classes;			/* find class */
classlast=classes;

while(classnext != NULL) {
	if(strcmp(classnext->name,classx) == 0) break;		/* found race */

	classnext=classnext->next;
}

usernext->userclass=classlast->next;
usernext->carryobjects=NULL;
usernext->last=userlast;

clientiplen=sizeof(struct sockaddr_in);			/* get ip address */
getpeername(socket,(struct sockaddr*)&clientip,&clientiplen);
strcpy(usernext->ipaddress,inet_ntoa(clientip.sin_addr));

UpdateUsersFile();		/* update users file */
return(0);
}

int AddNewRace(user *currentuser,race *newrace) {
race *raceptr;
race *racelast;

if(races == NULL) {
	races=calloc(1,sizeof(race));
	if(races == NULL) {
		SetLastError(currentuser,NO_MEM);
		return(-1);
	}

	raceptr=races;
}
else
{
	/* check if race exists */

	raceptr=races;

	while(raceptr != NULL) {
		racelast=raceptr;

		if(strcmp(raceptr->name,newrace->name) == 0) {
			SetLastError(currentuser,RACE_EXISTS);
			return(-1);
		}

		raceptr=raceptr->next;
	}

	racelast->next=calloc(1,sizeof(race));		/*  add new */
	if(racelast->next == NULL) {
		SetLastError(currentuser,NO_MEM);
		return(-1);
	}

	raceptr=racelast->next;
}

memcpy(raceptr,newrace,sizeof(race));		/* add new race */

SetLastError(currentuser,NO_ERROR);
return(0);
}

int AddNewClass(user *currentuser,class *newclass) {
class *classptr;
class *classlast;

if(classes == NULL) {
	classes=calloc(1,sizeof(class));
	if(classes == NULL) {
		SetLastError(currentuser,NO_MEM);
		return(-1);
	}

	classptr=classes;
}
else
{
	/* check if class exists */

	classptr=classes;

	while(classptr != NULL) {
		classlast=classptr;

		if(strcmp(classptr->name,newclass->name) == 0) {
			SetLastError(currentuser,CLASS_EXISTS);
			return(-1);
		}

		classptr=classptr->next;
	}

	classlast->next=calloc(1,sizeof(class));		/*  add new */
	if(classlast->next == NULL) {
		SetLastError(currentuser,NO_MEM);
		return(-1);
	}

	classptr=classlast->next;
}

memcpy(classptr,newclass,sizeof(class));		/* add new class */

SetLastError(currentuser,NO_ERROR);
return(0);
}

user *GetUserPointerByName(char *name) {
user *usernext;

/* renaming user */
usernext=users;

while(usernext != NULL) {

	if(regexp(usernext->name,name) == TRUE) return(usernext);		/* found user */

	usernext=usernext->next;
}

return(NULL);
}

user *FindFirstUser(void) {
return(users);
}

user *FindNextUser(user *last) {
return(last->next);
}

race *FindFirstRace(void) {
return(races);
}

race *FindNextRace(race *last) {
return(last->next);
}

char *GetPointerToMaleTitles(int level) {
return(MaleUserLevelNames[level]);
}

char *GetPointerToFemaleTitles(int level) {
return(MaleUserLevelNames[level]);
}

void AttackUser(int RoomNumber,int roommonster) {
user *usernext=users;
int HitPoints;
char *OutputMessage[BUF_SIZE];

while(usernext != NULL) {
	if(usernext->room == RoomNumber) {		/* user is in room */
		HitPoints=rand() % (GetRoomMonsterEvil(RoomNumber,roommonster) + 1) - 0;		/* random damage */

		sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",GetRoomMonsterName(RoomNumber,roommonster),usernext->name,HitPoints);
		SendMessageToAllInRoom(RoomNumber,OutputMessage);

		usernext->staminapoints -= HitPoints;

		UpdateUser(usernext,usernext->name,"",0,0,"",0,usernext->staminapoints,0,0,"","",0);

	}

	usernext=usernext->next;
}

return;
}

void DisconnectAllUsers(void) {
user *usernext=users;

while(usernext != NULL) {
	close(usernext->handle);		/* close tcp connections */
	usernext=usernext->next;
}

return;
}

