#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

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
char *ObjectConfigurationFile="config/reset.mud";
char *GoodbyeMessage="Goodbye.";
char *PlayAgainMessage="Play again (y/n)?";
int UserUpdated;

int BanUserByName(user *currentuser,char *username) {
user *UserPtr;

if(currentuser->status < WIZARD) {		/* not yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

UserPtr=users;		/* find user */

while(UserPtr != NULL) {
	if(regexp(UserPtr->name,username) == TRUE) {	/* found ip address */
		BanUserByIPAddress(currentuser,UserPtr->ipaddress);
		UpdateBanFile();
		return(0);
	}

	UserPtr=UserPtr->next;
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
char *OutputMessage[BUF_SIZE];

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
		sprintf(OutputMessage,"%s\r\n",bannext->ipaddress);

		send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);
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

int ForceUser(user *currentuser,char *username,char *command) {
user *UserPtr;

if(currentuser->status < WIZARD) {             /* can't do this unless wizard or higher level */
	SetLastError(currentuser,NOT_YET);
	return(0);
}

UserPtr=users;

while(UserPtr != NULL) {
	if(regexp(username,UserPtr->name) == TRUE && UserPtr->loggedin == TRUE) return(ExecuteCommand(UserPtr,command));    /* do command */ 
		
	UserPtr=UserPtr->next;
}

return(0);
}

/* give object to user */

int GiveObjectToUser(user *currentuser,char *username,char *objectname) {
user *UserPtr;
roomobject *RoomObjectPtr;
roomobject *ourobjectlast;
roomobject *temp;
int found=0;
int  ObjectFound=0;
roomobject *ourobject;

/*
* find user
*/

UserPtr=users;

while(UserPtr != NULL) {
	if(regexp(UserPtr->name,username) == TRUE) {		/* found object */
		found=TRUE;
		break;
	}

	UserPtr=UserPtr->next;
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
	if(regexp(ourobject->name,objectname) == TRUE) {		/* found object */
	
		if(UserPtr->carryobjects != NULL) {				/* find end */				
			UserPtr->carryobjects=calloc(1,sizeof(roomobject));
			if(RoomObjectPtr->next == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);	
			}

			UserPtr->carryobjects_last=UserPtr->carryobjects;
		}
		else
		{						
			UserPtr->carryobjects_last->next=calloc(1,sizeof(roomobject));	/* allocate objects */ 
			if(UserPtr->carryobjects_last->next == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}

			UserPtr->carryobjects_last=UserPtr->carryobjects_last->next;
		}    


		memcpy(UserPtr->carryobjects_last,ourobject,sizeof(roomobject));	/* copy data */
	
		/* remove object from source player's inventory */
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
* kill user
*/
int KillUser(user *currentuser,char *username) {
room *roomnext;
user *UserPtr;
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

UserPtr=users;
while(UserPtr != NULL) {
	if(regexp(UserPtr->name,username) == TRUE && UserPtr->loggedin == TRUE) {		/* found user */
		found=TRUE;

		if(currentuser->status < UserPtr->status ) {  /* wizards can't be killed */
			SetLastError(currentuser,KILL_WIZARD);
			return(-1);
		}

		if(UserPtr->gender == MALE) {
			sprintf(OutputMessage,"You were given the finger of death by %s the %s\r\n",currentuser->name,MaleUserLevelNames[currentuser->status]);
		}
		else
		{
			sprintf(OutputMessage,"You were given the finger of death by %s the %s\r\n",currentuser->name,FemaleUserLevelNames[currentuser->status]);
		}

		send(UserPtr->handle,OutputMessage,strlen(OutputMessage),0);
		close(UserPtr->handle);

		UserPtr->next=UserPtr->last;
		free(UserPtr);

		UpdateUser(UserPtr,username,"",0,0,"",0,0,0,0,"","",0);          /* remove user */

		sprintf(UserInventoryFile,"config/%s.inv",UserPtr->name);		/* get absolute path of user inventory */
		unlink(UserInventoryFile);		/* delete inventory file */

		return(-1);		
	}

	UserPtr=UserPtr->next;
}

/*
* if monster
*/

found=FALSE;

for(count=0;count<currentroom->monstercount;count++) {
	if(regexp(username,currentroom->roommonsters[count].name) == TRUE) {		/* found monster */
		DeleteMonster(currentroom->id,count);
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
user *UserPtr;
char *OutputMessage[BUF_SIZE];

UserPtr=users;

while(UserPtr != NULL) {

	if(UserPtr->room == currentuser->room) {	/* in same room */
		sprintf(OutputMessage,"*%s %s\r\n",currentuser->name,message);

		SendMessageToAllInRoom(UserPtr->room,OutputMessage);	/* send message */
	}

	UserPtr=UserPtr->next;
}

return(0);
}

/*
* disconnect user
*/

void quit(user *currentuser) {
char *OutputMessage[BUF_SIZE];

sprintf(OutputMessage,"%s has disconnected\r\n",currentuser->name);

SendMessageToAllInRoom(currentuser->room,OutputMessage);
currentuser->loggedin=FALSE; /* mark as logged out */

send(currentuser->handle,GoodbyeMessage,strlen(GoodbyeMessage),0);

DisconnectUser(currentuser);		/* disconnect user */
}

/*
* send private message to someone
*/

int SendMessageToAllInRoom(int room,char *message) {
user *UserPtr;

UserPtr=users;

while(UserPtr != NULL) {
	if((UserPtr->room == room) && (UserPtr->loggedin == TRUE)) send(UserPtr->handle,message,strlen(message),0);	/* found user */

	UserPtr=UserPtr->next;
}

return(0);
}

/*
* send private message to someone
*/

int SendMessage(user *currentuser,char *username,char *message) {
char *OutputMessage[BUF_SIZE];
user *UserPtr;
int found=FALSE;

UserPtr=users;

while(UserPtr != NULL) {
	if(regexp(username,UserPtr->name) == TRUE) {		/* found user */

		if(currentuser->flags & USER_INVISIBLE) {
			sprintf(OutputMessage,"Somebody whispers, %s\r\n",username,message);
		}
		else
		{
			sprintf(OutputMessage,"[%s] %s\r\n",username,message);
		}

		found=TRUE;
		send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);
	}

	UserPtr=UserPtr->next;
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

int TakeObject(user *currentuser,char *username,char *object) {
user *UserPtr;
roomobject *RoomObjectPtr;
roomobject *RoomObjectLast;
roomobject *myobj;
int found=0;
int  ObjectFound=0;

if(currentuser->status < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/*
* find user
*/

UserPtr=users;

while(UserPtr != NULL) {
	if(regexp(UserPtr->name,username) == TRUE) {		/* found user */
		found=TRUE;
		break;
	}

	UserPtr=UserPtr->next;
}

if(found == FALSE) {
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/*
* find object
*/

RoomObjectPtr=UserPtr->carryobjects;

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
	
		if(RoomObjectPtr == UserPtr->carryobjects) {		/* first object */
			RoomObjectPtr=RoomObjectPtr->next;
	
			free(UserPtr->carryobjects);   
			UserPtr->carryobjects=RoomObjectPtr;
		}
		else if(RoomObjectPtr != UserPtr->carryobjects && RoomObjectPtr->next != NULL) {      
			RoomObjectLast->next=RoomObjectPtr->next;	/* skip over over object */        
			free(RoomObjectPtr);
		}
		else if(RoomObjectPtr == UserPtr->carryobjects && RoomObjectPtr->next != NULL) {		/* last object */          
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
user *UserPtr;
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

UserPtr=users;
while(UserPtr != NULL) {

/*
* if the new entry value is 0 then it is ignored and the value is unchanged,
if the stamina points are 0 the user is killed and will not be included in the updated file
*/

	if(regexp(UserPtr->name,username) == TRUE) {			/* found user */

		strcpy(UserPtr->name,username);
		if(*password) strcpy(UserPtr->password,password);
	
		UserPtr->homeroom=homeroom;
		UserPtr->status=userlevel;
		UserPtr->magicpoints=magicpoints;

		UserPtr->flags=flags;

		UserPtr->staminapoints=staminapoints;

		/*
		* the user is dead, long live the user
		*/

		if(UserPtr->staminapoints <= 0 && (UserPtr->status < WIZARD)) {
			printf("User is DEAD\n");

			UserPtr->staminapoints=DEFAULT_STAMINAPOINTS;		/* reset user */
			UserPtr->magicpoints=DEFAULT_MAGICPOINTS;
			UserPtr->experiencepoints=0;
			UserPtr->status=NOVICE;
			UserPtr->homeroom=1;
			
			sprintf(OutputMessage,"%s was killed\n",UserPtr->name);
			SendMessageToAllInRoom(UserPtr->room,OutputMessage);

			DropObject(UserPtr,"*"); 		/* drop objects carried by user */

			UserUpdated=TRUE;

			send(currentuser->handle,PlayAgainMessage,strlen(PlayAgainMessage),0);
			
			return(-2);		/* signal that the player is dead */
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

			if(newlevel > UserPtr->status || newlevel < UserPtr->status) {		/* new level */
				UserPtr->status=newlevel;
	
				if(UserPtr->gender == MALE) {
					sprintf(OutputMessage,"You are now a %s!\n",MaleUserLevelNames[newlevel]);
				}
				else
				{
					sprintf(OutputMessage,"You are now a %s!\n",FemaleUserLevelNames[newlevel]);
				}
			}

			send(UserPtr->handle,OutputMessage,strlen(OutputMessage),0);
		}
		

		UserPtr->experiencepoints=experiencepoints;

		if(gender > 0) UserPtr->gender=gender;

		if(*racex) {
			racenext=races;				/* find race */

			while(racenext != NULL) {
				if(strcmp(racenext->name,racex) == 0) {		/* found race */
					UserPtr->race=racenext;
					break;
				}

				racenext=racenext->next;
			}
		}

		if(*classx) {
			classnext=classes;				/* find class */

			while(classnext != NULL) {
				if(strcmp(classnext->name,classx) == 0) {		/* found class */
					UserPtr->userclass=classnext;
					break;
				}
	
				classnext=classnext->next;
			}
		}

		if(*description) strcpy(UserPtr->desc,description);

		UserUpdated=TRUE;
	}

	UserPtr=UserPtr->next;
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
user *UserPtr;
roomobject *RoomObjectPtr;
race *racenext;
class *classnext;
char *InventoryFile[BUF_SIZE];
char *GenderString[BUF_SIZE];

handle=fopen(UserConfigurationFile,"w");
if(handle == NULL) return(-1);

UserPtr=users;

while(UserPtr != NULL) {

	racenext=UserPtr->race;				/* find race */
	classnext=UserPtr->userclass;

	if(UserPtr->gender == MALE) {		/* gender */
		strcpy(GenderString,"male");
	}
	else
	{
		strcpy(GenderString,"female");
	}

	fprintf(handle,"%s:%s:%d:%d:%s:%d:%d:%d:%s:%s:%s:%d\n",UserPtr->name,UserPtr->password,UserPtr->homeroom,UserPtr->status,\
						UserPtr->desc,UserPtr->magicpoints,UserPtr->staminapoints,UserPtr->experiencepoints, \
						GenderString,racenext->name,classnext->name,UserPtr->flags);

	/* update inventory file */

	sprintf(InventoryFile,"config/%s.inv",UserPtr->name);			/* get path */

	handleinv=fopen(InventoryFile,"w");
	if(handleinv != NULL) {		/* can't open */
		RoomObjectPtr=UserPtr->carryobjects;

		while(RoomObjectPtr != NULL) {
			fprintf(handleinv,"%s:%d:%d:%d:%d:%s\n",RoomObjectPtr->name,RoomObjectPtr->staminapoints,RoomObjectPtr->magicpoints,\

			RoomObjectPtr->attackpoints,RoomObjectPtr->generateprob,RoomObjectPtr->desc);			

			RoomObjectPtr=RoomObjectPtr->next;
		}

		fclose(handleinv);
	}
		
	UserPtr=UserPtr->next;
}

fclose(handle);
return(0);
}

/*
* set user points (magic/stamina/experience)
*/

int SetUserPoints(user *currentuser,char *username,char *amountstr,int which) {
user *UserPtr;
int amount;

UserPtr=users;

while(UserPtr != NULL) {

	if(regexp(UserPtr->name,username) == TRUE) {	/* if user found */

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
			return(UpdateUser(currentuser,UserPtr->name,"",0,0,"",UserPtr->magicpoints+amount,0,0,0,"","",0));
		}
		else if(which == STAMINAPOINTS) {
			return(UpdateUser(currentuser,UserPtr->name,"",0,0,"",0,UserPtr->staminapoints+amount,0,0,"","",0));
		}
		else if(which == EXPERIENCEPOINTS) {
			return(UpdateUser(currentuser,UserPtr->name,"",0,0,"",0,0,UserPtr->experiencepoints+amount,0,"","",0));
		}
	}

	UserPtr=UserPtr->next;
}

SetLastError(currentuser,UNKNOWN_USER);		/* user not found */
return(-1);
}

/*
* set user level */

int SetUserLevel(user *currentuser,char *username,char *levelstr) {
user *UserPtr;
int level;

if(currentuser->status < WIZARD) {     /* not wizard */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

UserPtr=users;
while(UserPtr != NULL) {

	if(regexp(UserPtr->name,username) == TRUE) {		/* found user */
	
		if((char) *levelstr == '+') {			/* add points */
			sscanf(levelstr,"+%d",&level);

			if(UserPtr->status+level > 12) {
				SetLastError(currentuser,INVALID_LEVEL);
				return(-1);
			}
	
			 if(UserPtr->status+level > currentuser->status) {		/* can't set level above own level */
				SetLastError(currentuser,NOT_YET);
				return(-1);
			 }

			UpdateUser(currentuser,username,"",0,UserPtr->status+level,"",0,0,0,0,"","",0);   /* set level */
			return(0);
		}
		else if((char) *levelstr == '-') {			/* subtract points */
			sscanf(levelstr,"-%d",&level);

			if((UserPtr->status-level <= 0) || (UserPtr->status+level <= 0)) {
				SetLastError(currentuser,INVALID_LEVEL);
				return(-1);
			}
	
			UpdateUser(currentuser,username,"",0,UserPtr->status-level,"",0,0,0,0,"","",0);   /* set level */
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

	UserPtr=UserPtr->next;
}

return(0);
}


/* set gender */
int SetUserGender(user *currentuser,char *username,char *gender) {

if(currentuser->status < WIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(strcmp(gender,"male") == 0) {
	return(UpdateUser(currentuser,username,"",0,0,"",0,0,0,MALE,"","",0));
}
else if(strcmp(gender,"female") == 0) {
	return(UpdateUser(currentuser,username,"",0,0,"",0,0,0,FEMALE,"","",0));
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
	else if(strcmp(RaceTokens[0],"intelligence") == 0) {	/* race points used */
		racenext->intelligence=atoi(RaceTokens[1]);
		continue;			
	}
	else if(strcmp(RaceTokens[0],"strength") == 0) {
		racenext->strength=atoi(RaceTokens[1]);
		continue;			
	}
	else if(strcmp(RaceTokens[0],"wisdom") == 0) {
		racenext->wisdom=atoi(RaceTokens[1]);
		continue;			
	}  
	else if(strcmp(RaceTokens[0],"dexterity") == 0) {
		racenext->dexterity=atoi(RaceTokens[1]);
		continue;			
	}
	else if(strcmp(RaceTokens[0],"luck") == 0) {
		racenext->luck=atoi(RaceTokens[1]);
		continue;			
	}
	else if(strcmp(RaceTokens[0],"magic") == 0) {
		racenext->magic=atoi(RaceTokens[1]);
		continue;			
	}  
	else if(strcmp(RaceTokens[0],"agility") == 0) {
		racenext->agility=atoi(RaceTokens[1]);
		continue;			
	}
	else if(strcmp(RaceTokens[0],"stamina") == 0) {
		racenext->stamina=atoi(RaceTokens[1]);
		continue;			
	}
	else if(strcmp(RaceTokens[0],"end") == 0) {
		;;
	}
	else
	{
		printf("\nmud: %d: uknown configuration option %s in %s\n",LineCount,RaceTokens[0],RaceConfigurationFile);		/* unknown configuration option */
		ErrorCount++;
	}
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
user *UserPtr;
FILE *handle;
int LineCount;
char *UserTokens[BUF_SIZE][BUF_SIZE];
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

	TokenizeLine(LineBuffer,UserTokens,":\n");				/* tokenize line */

	if(users == NULL) {			/* first user */
		users=calloc(1,sizeof(user));
		if(users == NULL) {
			perror("\nmud:");
			exit(NOMEM);
		}

		UserPtr=users;
	}
	else
	{
		UserPtr->next=calloc(1,sizeof(user));
		UserPtr=UserPtr->next;

		if(UserPtr == NULL) {
			perror("\nmud:");
			exit(NOMEM);
		}

	}

	strcpy(UserPtr->name,UserTokens[USERNAME]);
	strcpy(UserPtr->password,UserTokens[PASSWORD]);
	UserPtr->homeroom=atoi(UserTokens[HOMEROOM]);
	UserPtr->status=atoi(UserTokens[USERLEVEL]);
	strcpy(UserPtr->desc,UserTokens[DESCRIPTION]);
	UserPtr->magicpoints=atoi(UserTokens[MAGICPOINTS]);
	UserPtr->staminapoints=atoi(UserTokens[STAMINAPOINTS]);
	UserPtr->experiencepoints=atoi(UserTokens[EXPERIENCEPOINTS]);
	UserPtr->gender=atoi(UserTokens[GENDER]);
	UserPtr->handle=0;
	UserPtr->flags=atoi(UserTokens[USERFLAGS]);
	UserPtr->next=NULL;

	userrace=races;		/* load race */
	racelast=races;

	while(userrace != NULL) {
		if(strcmp(userrace->name,UserTokens[RACE]) == 0) {		/* FOund race */
			UserPtr->race=racelast;
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
	if(strcmp(userclass->name,UserTokens[CLASS]) == 0) {		/* FOund class */
		UserPtr->userclass=classlast;
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

int wall(user *currentuser,char *message) {
user *UserPtr;
char *OutputMessage[BUF_SIZE];

if(currentuser->status < WIZARD) {		/* only wizard or higher users can send global message */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

UserPtr=users;

while(UserPtr != NULL) {
	
	if(UserPtr->loggedin == TRUE) {
		sprintf(OutputMessage,"[GLOBAL MESSAGE] %s\n",message);

		send(UserPtr->handle,OutputMessage,strlen(OutputMessage),0);			/* send message to every user */
	}

	UserPtr=UserPtr->next;
}

return(0);
}

int go(user *currentuser,int RoomNumber) {
room *roomnext;
char *OutputMessage[BUF_SIZE];

if(RoomNumber == 0) {		/* invalid room */
	SetLastError(currentuser,BAD_DIRECTION);
	return(-1);
}

if(currentuser->room != RoomNumber) {	/* send leaving message */
	sprintf(OutputMessage,"%s has left\r\n",currentuser->name);
	SendMessageToAllInRoom(currentuser->room,OutputMessage);
}

if(GetRoomAttributes(RoomNumber) & ROOM_PRIVATE) {
	SetLastError(currentuser,BAD_DIRECTION);
	return(-1);
}	

strcpy(currentuser->roomname,GetRoomName(RoomNumber));

currentuser->room=RoomNumber;
currentuser->roomptr=GetRoomPointer(RoomNumber); 		/* save pointer to current room */

sprintf(OutputMessage,"%s has entered\r\n",currentuser->name);
SendMessageToAllInRoom(currentuser->room,OutputMessage);

look_command(currentuser,0,NULL);				/* look at new room */

if(GetRoomAttributes(RoomNumber) & ROOM_DEAD) {
	UpdateUser(currentuser,currentuser->name,currentuser->password,currentuser->homeroom,currentuser->status,currentuser->desc,currentuser->magicpoints,0,currentuser->experiencepoints,currentuser->gender,currentuser->race,currentuser->userclass,currentuser->flags);
	return(-1);
}	

return(0);
}

int MoveObject(user *currentuser,char *ObjectName,int RoomNumber) {
roomobject *RoomObjectPtr;
roomobject *DestinationObject;
int destination;
user *UserPtr;
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
	SetLastError(currentuser,OBJECT_NOT_FOUND);
	return(-1);
}

DestinationRoom=GetRoomPointer(RoomNumber);		/* point to room */

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

return(0);
}


int LoginUser(int messagesocket,char *username,char *password) {
char *encryptedpassword[BUF_SIZE];
char *RaceTokens[BUF_SIZE][BUF_SIZE];
FILE *handle;
char *UserLine[BUF_SIZE];
user *UserPtr;
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

UserPtr=users;
userlast=users;

while(UserPtr != NULL) {
/* check username and password */
	if(strcmp(username,UserPtr->name) == 0 && strcmp(encryptedpassword,UserPtr->password) == 0) {
		strcpy(UserPtr->ipaddress,ipaddress);	/* IP address */

		UserPtr->loggedin=TRUE;		/* user is logged in */
		UserPtr->handle=messagesocket;		/* TCP socket */
		UserPtr->room=UserPtr->homeroom;	/* room */
		UserPtr->roomptr=GetRoomPointer(UserPtr->homeroom);

		/*
		* load user inventory
		*/

		sprintf(UserInventoryFile,"config/%s.inv",UserPtr->name);		/* get absolute path of user inventory */

		UserPtr->carryobjects=calloc(1,sizeof( roomobject));		/* allocate objects */
		if(UserPtr->carryobjects == NULL) {
			PrintError(messagesocket,NO_MEM);
			return(-1);		/* can't allocate */
		}

		RoomObjectPtr=UserPtr->carryobjects;

		handle=fopen(UserInventoryFile,"rb");
		if(handle != NULL) {
			while(!feof(handle)) {
				fgets(UserLine,BUF_SIZE,handle);	
				if(feof(handle)) break;
	
				RemoveNewLine(UserLine);		/* remove newline character */

				TokenizeLine(UserLine,RaceTokens,":");		/* tokenize line */
				strcpy(RoomObjectPtr->name,RaceTokens[OBJECT_NAME]);
				RoomObjectPtr->staminapoints=atoi(RaceTokens[OBJECT_STAMINAPOINTS]);
				RoomObjectPtr->magicpoints=atoi(RaceTokens[OBJECT_MAGICPOINTS]);
				RoomObjectPtr->attr=atoi(RaceTokens[OBJECT_ATTR]);
				RoomObjectPtr->attackpoints=atoi(RaceTokens[OBJECT_ATTACKPOINTS]);
				RoomObjectPtr->generateprob=atoi(RaceTokens[OBJECT_GENERATEPROB]);
				strcpy(RoomObjectPtr->desc,RaceTokens[OBJECT_DESCRIPTION]);
				strcpy(RoomObjectPtr->owner,RaceTokens[OBJECT_OWNER]);

				RoomObjectPtr->next=calloc(1,sizeof( roomobject));		/* allocate objects */
				if(UserPtr->carryobjects == NULL) {
					PrintError(messagesocket,NO_MEM);
					return(-1);		/* can't allocate */
				}

				RoomObjectPtr=RoomObjectPtr->next;
			}
		
			RoomObjectPtr->next=NULL;

			fclose(handle);

			if(go(UserPtr,UserPtr->homeroom) == -1) return(-1);
		}

		return(0);
	}


	UserPtr=UserPtr->next;
}

return(-1);
}

int CreateUser(int socket,char *name,char *pass,int gender,char *description,char *racex,char *classx) {
user *UserPtr;
user *userlast;
struct sockaddr_in clientip;
socklen_t clientiplen;
char *ipaddress[BUF_SIZE];
race *racenext;
class *classnext;
race *racelast;
class *classlast;

if(UserPtr == NULL) {
	users=calloc(1,sizeof(user));		/* add to end */
	UserPtr=users;
	userlast=users;
}
else
{
	userlast=UserPtr;
	UserPtr=users;

	while(UserPtr != NULL) {
		userlast=UserPtr;
		UserPtr=UserPtr->next;
	}

	userlast->next=calloc(1,sizeof(user));		/* add to end */
	if(userlast->next == NULL) return(-1);
}

UserPtr=userlast->next;

strcpy(UserPtr->name,name);
strcpy(UserPtr->password,pass);
strcpy(UserPtr->desc,description);
UserPtr->status=NOVICE;
UserPtr->homeroom=1;
UserPtr->room=1;
UserPtr->gender=gender;
UserPtr->magicpoints=DEFAULT_MAGICPOINTS;
UserPtr->staminapoints=DEFAULT_STAMINAPOINTS;
UserPtr->experiencepoints=0;
UserPtr->loggedin=TRUE;
UserPtr->handle=socket;
UserPtr->flags=0;
UserPtr->roomptr=GetRoomPointer(1);

racenext=races;			/* find race */
racelast=races;

while(racenext != NULL) {
	if(strcmp(racenext->name,racex) == 0) {		/* found race */
		UserPtr->race=racelast->next;
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

UserPtr->userclass=classlast->next;
UserPtr->carryobjects=NULL;
UserPtr->last=userlast;

clientiplen=sizeof(struct sockaddr_in);			/* get ip address */
getpeername(socket,(struct sockaddr*)&clientip,&clientiplen);
strcpy(UserPtr->ipaddress,inet_ntoa(clientip.sin_addr));

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
user *UserPtr;

/* renaming user */
UserPtr=users;

while(UserPtr != NULL) {

	if(regexp(UserPtr->name,name) == TRUE) return(UserPtr);		/* found user */

	UserPtr=UserPtr->next;
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
user *UserPtr=users;
int HitPoints;
char *OutputMessage[BUF_SIZE];

while(UserPtr != NULL) {
	if(UserPtr->room == RoomNumber) {		/* user is in room */
		HitPoints=rand() % (GetRoomMonsterEvil(RoomNumber,roommonster) + 1) - 0;		/* random damage */

		sprintf(OutputMessage,"%s attacks %s causing %d points of damage\r\n",GetRoomMonsterName(RoomNumber,roommonster),UserPtr->name,HitPoints);
		SendMessageToAllInRoom(RoomNumber,OutputMessage);

		UserPtr->staminapoints -= HitPoints;

		UpdateUser(UserPtr,UserPtr->name,"",0,0,"",0,UserPtr->staminapoints,0,0,"","",0);

	}

	UserPtr=UserPtr->next;
}

return;
}

void DisconnectAllUsers(void) {
user *UserPtr=users;

while(UserPtr != NULL) {
	close(UserPtr->handle);		/* close tcp connections */
	UserPtr=UserPtr->next;
}

return;
}


int PickUpObject(user *currentuser,char *ObjectName) {
roomobject *UserCarryObjectsPtr;
char *ErrorMessage[BUF_SIZE];

/* check if user is already carrying this object */

UserCarryObjectsPtr=currentuser->carryobjects;

while(UserCarryObjectsPtr != NULL) {

	if(regexp(UserCarryObjectsPtr->name,ObjectName) == TRUE) {	/* already picked up object */
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

	strcpy(currentuser->carryobjects_last->name,ObjectName);		/* add item */

	if(UserCarryObjectsPtr->magicpoints > 0) {
		sprintf(ErrorMessage,"You have gained %d magic points!\r\n",UserCarryObjectsPtr->magicpoints);
		send(currentuser->handle,ErrorMessage,strlen(ErrorMessage),0);
	}

	if(UserCarryObjectsPtr->staminapoints > 0) {
		sprintf(ErrorMessage,"You have gained %d stamina points!\r\n",UserCarryObjectsPtr->staminapoints);
		send(currentuser->handle,ErrorMessage,strlen(ErrorMessage),0);
	}  

	UserCarryObjectsPtr->magicpoints=0;
	UserCarryObjectsPtr->staminapoints=0;
	UserCarryObjectsPtr->next=NULL;
}

SetLastError(currentuser,NO_ERROR);
return(0);
}


int DropObject(user *currentuser,char *ObjectName) {
roomobject *UserCarryObjectsPtr;
roomobject *UserCarryObjectsLast;
room *CurrentRoom;
char *ObjectsList[BUF_SIZE];
int found=FALSE;

/* check permissions */

CurrentRoom=currentuser->roomptr;

if(currentuser->status < ARCHWIZARD) {
	if((CurrentRoom->attr & ROOM_CREATE_OWNER) == 0) {
		SetLastError(currentuser,CANT_CREATE_OBJECTS_HERE);  
		return(-1);
	}
	else
	{
		if((CurrentRoom->attr & ROOM_CREATE_PUBLIC) == 0) {
			SetLastError(currentuser,CANT_CREATE_OBJECTS_HERE);  
			return(-1);
		}
	}
}

UserCarryObjectsPtr=currentuser->carryobjects;		/* point to carried objects */
UserCarryObjectsLast=UserCarryObjectsPtr;

while(UserCarryObjectsPtr != NULL) {
	
	if(regexp(UserCarryObjectsPtr->name,ObjectName) == TRUE) {	/* found object */
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
			CurrentRoom->roomobjects_last=calloc(1,sizeof(roomobject));	/* allocate objects */
			if(CurrentRoom->roomobjects_last == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);  
				return(-1);
			}
		}

		CurrentRoom->roomobjects_last->next=CurrentRoom->roomobjects_last;
	}

	CurrentRoom->roomobjects_last=CurrentRoom->roomobjects_last->next;

	memcpy(CurrentRoom->roomobjects_last,UserCarryObjectsPtr,sizeof(roomobject));	/* copy object */
	CurrentRoom->roomobjects_last->id=GetNextObjectNumber();	/* generate new ID number for dropped object */
				
	if(UserCarryObjectsPtr == currentuser->carryobjects) {		/* first object */
		UserCarryObjectsPtr=UserCarryObjectsPtr->next;

		free(currentuser->carryobjects);   
		currentuser->carryobjects=UserCarryObjectsPtr;
		found=TRUE;
	}

	if(UserCarryObjectsPtr->next == NULL) {		/* last object */
		found=TRUE;
	      	free(UserCarryObjectsPtr);	
	}

	if(UserCarryObjectsPtr != currentuser->carryobjects && UserCarryObjectsPtr->next != NULL) {      
		UserCarryObjectsLast->next=UserCarryObjectsPtr->next;	/* skip over over object */
		free(UserCarryObjectsPtr);
	}

	found=TRUE;

	UserCarryObjectsLast=UserCarryObjectsPtr;
	UserCarryObjectsPtr=UserCarryObjectsPtr->next;
}

if(found == FALSE) {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  	/* not found */
	return(-1);
}

SetLastError(currentuser,NO_ERROR);
return(0);
}

class *FindFirstClass(void) {
return(classes);
}

class *FindNextClass(class *previous) {
return(previous->next);
}

