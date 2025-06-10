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
char *banconf[BUF_SIZE];
ban *bans=NULL;
race *races=NULL;
class *classes=NULL;

/* titles */
char *maleusertitles[] = {"","Novice","Warrior","Hero","Champion","Superhero","Enchanter","Sorceror","Necromancer", \
			"Legend","Wizard","Arch Wizard","Dungeon Master" };

char *femaleusertitles[] = {"","Novice","Warrior","Heroine","Champion","Superheroine","Enchanteress","Sorceroress", \
			"Legend","Witch","Arch Witch","Dungeon Master" };

char *banconf[BUF_SIZE];
char *banlistprompt="Press ENTER to see more bans or q to quit:";
char *banrel="/config/ban.mud";
char *classconf[BUF_SIZE];
char *classrel="/config/classes.mud";
char *userrel="/config/users.mud";
char *nouser="Unknown user\r\n";
char *racerel="/config/races.mud";
char *usersconf[BUF_SIZE];
char *resetconf[BUF_SIZE];
int userupdated;

int userban(user *currentuser,char *username) {
user *usernext;
room *currentroom;
char *buf[BUF_SIZE];
char *b;

if(currentuser->status < WIZARD) {		/* not yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

usernext=users;		/* find user */

while(usernext != NULL) {
	if(regexp(usernext->name,username) == TRUE) {	/* found ip address */

		banip(currentuser,usernext->ipaddress);
		updatebanfile();
		return(0);
	}

	usernext=usernext->next;
}

SetLastError(currentuser,UNKNOWN_USER);
return(-1);
}

int banip(user *currentuser,char *ipaddr) {
int count;
char *b;
ban *banlist;
char *buf[BUF_SIZE];

if(currentuser->status < WIZARD) {		/* not yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

banlist=bans;

while(banlist != NULL) {
	if(*banlist->ipaddress && strcmp(banlist->ipaddress,ipaddr) == 0) {		/* ip address already banned */
		SetLastError(currentuser,ALREADY_BANNED);
	}

	banlist=banlist->next;
}

banlist=bans;

while(banlist->next != NULL) banlist=banlist->next;

banlist->next=calloc(1,sizeof(ban));	/* add new link */
if(banlist->next == NULL) {		/* can't allocate */
	SetLastError(currentuser,NO_MEM);
	return(-1);
}

banlist=banlist->next;
strcpy(banlist->ipaddress,ipaddr);
banlist->next=NULL;

return(0);
}

int updatebanfile(void) {
FILE *handle;
ban *bannext;

handle=fopen(banconf,"w");
if(handle == NULL) return(-1);		/* can't open */

bannext=bans;
while(bannext != NULL) {
	fprintf(handle,"%s\r\n",bannext->ipaddress);		/* write ip address */
	bannext=bannext->next;
}

fclose(handle);
return(0);
}

int listbans(user *currentuser,char *banname) {
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

		if(count++ > HELPPAGESIZE) {         /* show prompt */
			count=0;

			send(currentuser->handle,banlistprompt,strlen(banlistprompt),0);
			recv(currentuser,buf,1,0);
	
			b=buf;
			if(*b == 'q') return(0);
		}
	}
	
	bannext=bannext->next;
}

return(0);
}


int unbanip(user *currentuser,char *ipaddr) {
int count;
char *b;
ban *banlist;
char *buf[BUF_SIZE];
ban *banlast;

banlist=bans;

while(banlist != NULL) {
	banlast=banlist;

	if(*banlist->ipaddress && strcmp(banlist->ipaddress,ipaddr) == 0) {		/* ip address already banned */
		banlast->next=banlist->next;

		free(banlist);
		return(0);
	}

	banlist=banlist->next;
}

SetLastError(currentuser,UNKNOWN_USER);
return(-1);
}

int loadbans(void) {
ban *bannext;
char *linebuffer[BUF_SIZE];
int lc;
FILE *handle;

bannext=bans;
lc=0;

getcwd(linebuffer,BUF_SIZE);
sprintf(banconf,"%s/%s",linebuffer,banrel);		/* get absolute path of configuration file */


handle=fopen(banconf,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",banconf);
	return(-1);
}

do {
	fgets(linebuffer,BUF_SIZE,handle);		/* get and parse line */

	removenewline(linebuffer);		/* remove newline character */

	if((char) *linebuffer != '#')  {		/* skip comments */

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
	
		strcpy(bannext->ipaddress,linebuffer);

		bannext->next=calloc(1,sizeof(ban));	/* add new link */
		if(bannext->next == NULL) break;

		bannext=bannext->next;
	}

} while(!feof(handle));

fclose(handle);
return(0);
}

int checkban(char *name) {
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

int force(user *currentuser,char *u,char *c) {
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
	if(regexp(u,usernext->name) == TRUE && usernext->loggedin == TRUE) return(docommand(usernext,c));    /* do command */ 
		
	usernext=usernext->next;
}

return(0);
}

/*
* give object to user
*/

int give(user *currentuser,char *u,char *o) {
user *usernext;
roomobject *objnext;
roomobject *ourobjectlast;
roomobject *temp;
int found=0;
int  objfound=0;
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

		objnext=usernext->carryobjects;
	
		if(objnext != NULL) {				/* find end */				
			while(objnext->next != NULL) objnext=objnext->next; 

			objnext->next=calloc(1,sizeof(roomobject));
			if(objnext->next == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);	
			}

			objnext=objnext->next;
		}
		else
		{						
			usernext->carryobjects=calloc(1,sizeof(roomobject));	/* allocate objects */ 
			objnext=usernext->carryobjects;

			if(objnext == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}
		}    


		memcpy(objnext,ourobject,sizeof(roomobject));	/* copy data */
	
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

		objfound=TRUE;
	}

	ourobjectlast=ourobject;
	ourobject=ourobject->next;
}

if(objfound == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER); /* no object found */
	return(-1);
}

return(0);
}

/*
* display inventory
*/

int inventory(user *currentuser,char *u) {
char *whichuser[BUF_SIZE];
char *buf[BUF_SIZE];
roomobject *roomnext;
user *usernext;
roomobject *objnext;

if(!*u) {
	strcpy(whichuser,currentuser->name);   /* use default user */

}
else
{

	if(currentuser->status < WIZARD) {		/* can't do this yet */
		SetLastError(currentuser,NOT_YET);
		return(0);
	}

	strcpy(whichuser,u);
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

		objnext=usernext->carryobjects;

		while(objnext != NULL) {
			send(currentuser->handle,objnext->name,strlen(objnext->name),0);	/* display objects in inventory */
			send(currentuser->handle," ",1,0);
	
			objnext=objnext->next;
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
int killuser(user *currentuser,char *u) {
room *roomnext;
user *usernext;
monster *monsternext;
char *buf[BUF_SIZE];
int found=FALSE;
int count;
room *currentroom;
char *userinventoryfile[BUF_SIZE];
char *cwd[BUF_SIZE];

getcwd(cwd,BUF_SIZE);			/* get current directory */

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

usernext=users;
while(usernext != NULL) {
	if(regexp(usernext->name,u) == TRUE && usernext->loggedin == TRUE) {		/* found user */
		found=TRUE;

		if(currentuser->status < usernext->status ) {  /* wizards can't be killed */
			SetLastError(currentuser,KILL_WIZARD);
			return(-1);
		}

		if(usernext->gender == MALE) {
			sprintf(buf,"You were given the finger of death by %s the %s\r\n",currentuser->name,maleusertitles[currentuser->status]);
		}
		else
		{
			sprintf(buf,"You were given the finger of death by %s the %s\r\n",currentuser->name,femaleusertitles[currentuser->status]);
		}

		send(usernext->handle,buf,strlen(buf),0);
		close(usernext->handle);

		usernext->next=usernext->last;
		free(usernext);

		updateuser(usernext,u,"",0,0,"",0,0,0,0,"","",0);          /* remove user */

		sprintf(userinventoryfile,"%s/config/%s.inv",cwd,usernext->name);		/* get absolute path of user inventory */

		unlink(userinventoryfile);		/* delete inventory file */

		free(buf);
		return(-1);		
	}

	usernext=usernext->next;
}

/*
* if monster
*/

found=FALSE;

for(count=0;count<currentroom->monstercount;count++) {
	if(regexp(u,currentroom->roommonsters[count].name) == TRUE) {		/* found monster */
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

int pose(user *currentuser,char *msg) {
user *usernext;
char *buf[BUF_SIZE];

usernext=users;

while(usernext != NULL) {

	if(usernext->room == currentuser->room) {	/* in same room */
		sprintf(buf,"*%s %s\r\n",currentuser->name,msg);

		sendmudmessagetoall(usernext->room,buf);	/* send message */
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

sendmudmessagetoall(currentuser->room,buf);

currentuser->loggedin=FALSE; /* mark as logged out */

DisconnectUser(currentuser);		/* disconnect user */
}

int score(user *currentuser,char *u) {
user *usernext;
char *buf[BUF_SIZE];
char *name[BUF_SIZE];
int found;
void *titleptr;

found=FALSE;

if(!*u) {			/* find score for current user */
	strcpy(name,currentuser->name);
}
else
{
	if(currentuser->status < WIZARD) {		/* not yet */
		SetLastError(currentuser,NOT_YET);
		return(-1);
	}

	strcpy(name,u);
}

if(currentuser->gender == MALE) {		/* which user title */
	titleptr=maleusertitles[currentuser->status];
}
else
{
	titleptr=femaleusertitles[currentuser->status];
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

int sendmudmessagetoall(int room,char *msg) {
user *usernext;

usernext=users;

while(usernext != NULL) {
	if(usernext->room == room && usernext->loggedin == TRUE) send(usernext->handle,msg,strlen(msg),0);	/* found user */

	usernext=usernext->next;
}

return(0);
}

/*
* send private message to someone
*/

int sendmudmessage(user *currentuser,char *nick,char *msg) {
int count=0;
char *buf[BUF_SIZE];
user *usernext;

usernext=users;

while(usernext != NULL) {
	if(regexp(nick,usernext->name) == TRUE) {		/* found user */

		if(currentuser->flags & USER_INVISIBLE) {
			count++;
			sprintf(buf,"Somebody whispers, %s\r\n",nick,msg);
		}
		else
		{
			count++;
			sprintf(buf,"[%s] %s\r\n",nick,msg);
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

int take(user *currentuser,char *u,char *o) {
user *usernext;
roomobject *objnext;
roomobject *objlast;
roomobject *myobj;
int found=0;
int  objfound=0;
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
	if(regexp(usernext->name,u) == TRUE) {		/* found object */
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

objnext=usernext->carryobjects;
objlast=objnext;

while(objnext != NULL) {

	if(regexp(objnext->name,o) == TRUE) {		/* found object */
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
	
		memcpy(currentuser->carryobjects_last,objnext,sizeof(roomobject));	/* copy data */
	
		if(objnext == usernext->carryobjects) {		/* first object */
			objnext=objnext->next;
	
			free(usernext->carryobjects);   
			usernext->carryobjects=objnext;
		}

		if(objnext != usernext->carryobjects && objnext->next != NULL) {      
			objlast->next=objnext->next;	/* skip over over object */        
			free(objnext);
		}


		if(objnext == usernext->carryobjects && objnext->next != NULL) {		/* last object */          
			 free(objnext);	
		}
	
		objfound=TRUE;
	}

	objlast=objnext;
	objnext=objnext->next;
}

if(objfound == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER); /* no object found */
	return(-1);
}

return(0);
}

/*
* update user info
*/

int updateuser(user *currentuser,char *uname,char *upass,int uhome,int ulevel,char *udesc,int umpoints,int ustapoints,int uexpoints,int ugender,char *racex,char *classx,int uflags) {
int dead=0;
int count;
char *tokens[14][255];
user *usernext;
roomobject *objnext;
char *buf[BUF_SIZE];
int newlevel;
race *racenext;
class *classnext;
char c;
CONFIG config;

if(uhome < 0) uhome=0;			/* sanity check */
if(ulevel < 0) ulevel=0;
if(umpoints < 0) umpoints=0;
if(ustapoints < 0) ustapoints=0;
if(uexpoints < 0) uexpoints=0;

getconfigurationinformation(&config);

usernext=users;
while(usernext != NULL) {

/*
* if the new entry value is 0 then it is ignored and the value is unchanged,
if the stamina points are 0 the user is killed and will not be included in the updated file
*/

	if(regexp(usernext->name,uname) == TRUE) {			/* found user */

		strcpy(usernext->name,uname);
		if(*upass) strcpy(usernext->password,upass);
	
		if(uhome > 0) usernext->homeroom=uhome;
		if(ulevel > 0) usernext->status=ulevel;
		if(umpoints > 0) usernext->magicpoints=umpoints;

		if(uflags > 0) usernext->flags=uflags;

		usernext->staminapoints=ustapoints;

		/*
		* the user is dead, long live the user
		*/

		if(usernext->staminapoints <= 0 && (usernext->status < WIZARD)) {

			usernext->loggedin=FALSE;
			SetLastError(currentuser,GAME_OVER);

			usernext->staminapoints=DEFAULT_STAMINAPOINTS;		/* reset user */
			usernext->magicpoints=DEFAULT_MAGICPOINTS;
			usernext->experiencepoints=0;
			usernext->status=NOVICE;
			usernext->homeroom=1;

			sprintf(buf,"%s is dead\n",usernext->name);
			sendmudmessagetoall(usernext->room,buf);

			drop(usernext,"*"); 		/* drop objects carried by user */

			close(usernext->handle);   
			return(0);
		}

		/* adjust new level */

		 if(uexpoints > 0) {
			if(uexpoints < config.pointsforwarrior) newlevel=NOVICE;
			if((uexpoints >= config.pointsforwarrior) && (uexpoints < config.pointsforhero)) newlevel=WARRIOR;
			if((uexpoints >= config.pointsforhero) && (uexpoints < config.pointsforchampion)) newlevel=HERO;
			if((uexpoints >= config.pointsforchampion) && (uexpoints < config.pointsforsuperhero)) newlevel=CHAMPION;
			if((uexpoints >= config.pointsforsuperhero) && (uexpoints < config.pointsforenchanter)) newlevel=SUPERHERO;
			if((uexpoints >= config.pointsforenchanter) && (uexpoints < config.pointsforsorceror)) newlevel=ENCHANTER;
			if((uexpoints >= config.pointsforsorceror) && (uexpoints < config.pointsfornecromancer)) newlevel=SORCEROR;
			if((uexpoints >= config.pointsfornecromancer) && (uexpoints < config.pointsforlegend)) newlevel=NECROMANCER;
			if((uexpoints >= config.pointsforlegend) && (uexpoints < config.pointsforwizard)) newlevel=LEGEND;
			if((uexpoints >= config.pointsforwizard)) newlevel=WIZARD;

			if(newlevel > usernext->status || newlevel < usernext->status) {		/* new level */
				usernext->status=newlevel;
	
				if(usernext->gender == MALE) {
					sprintf(buf,"You are now a %s!\n",maleusertitles[newlevel]);
				}
				else
				{
					sprintf(buf,"You are now a %s!\n",femaleusertitles[newlevel]);
				}
			}

			send(usernext->handle,buf,strlen(buf),0);
		}
		

		usernext->experiencepoints=uexpoints;

		if(ugender > 0) usernext->gender=ugender;

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

		if(*udesc) strcpy(usernext->desc,udesc);

		userupdated=TRUE;
	}

	usernext=usernext->next;
}

setdatabaseupdatedflag();			/* set database updated flag */
return(0);
}

/*
* update users file
*/

int updateusersfile(void) {
FILE *handle;
FILE *handleinv;
char *buf[BUF_SIZE];
user *usernext;
roomobject *objnext;
race *racenext;
class *classnext;

char *linebuffer[10];

handle=fopen(usersconf,"w");
if(handle == NULL) return(-1);

usernext=users;

while(usernext != NULL) {

	racenext=usernext->race;				/* find race */
	classnext=usernext->userclass;

	if(usernext->gender == MALE) {		/* gender */
		strcpy(buf,"male");
	}
	else
	{
		strcpy(buf,"female");
	}

	fprintf(handle,"%s:%s:%d:%d:%s:%d:%d:%d:%s:%s:%s:%d\n",usernext->name,usernext->password,usernext->homeroom,usernext->status,\
						usernext->desc,usernext->magicpoints,usernext->staminapoints,usernext->experiencepoints, \
						buf,racenext->name,classnext->name,usernext->flags);

	/* update inventory file */

	getcwd(buf,BUF_SIZE);

	sprintf(buf,"/config/%s.inv",usernext->name);			/* get path */

	handleinv=fopen(buf,"w");
	if(handleinv != NULL) {		/* can't open */
		objnext=usernext->carryobjects;

		while(objnext != NULL) {
			fprintf(handleinv,"%s:%d:%d:%d:%d:%s\n",objnext->name,objnext->staminapoints,objnext->magicpoints,\

			objnext->attackpoints,objnext->generateprob,objnext->desc);			

			objnext=objnext->next;
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

int setpoints(user *currentuser,char *u,char *amountstr,int which) {
user *usernext;
char c;
char *buf[BUF_SIZE];
int amount;

usernext=users;

while(usernext != NULL) {

	if(regexp(usernext->name,u) == TRUE) {	/* if user found */
		c=*amountstr;


		if(c == '+' || c == '-') {				/* adding/subtracting/setting points */
			strncpy(buf,amountstr+1,strlen(amountstr)-1);

			if(c == '+') {
				if(which == MAGICPOINTS) amount=usernext->magicpoints+atoi(buf);
				if(which == STAMINAPOINTS) amount=usernext->staminapoints+atoi(buf);
				if(which == EXPERIENCEPOINTS) amount=usernext->experiencepoints+atoi(buf);
			}

			if(c == '-') {
				if(which == MAGICPOINTS) amount=usernext->magicpoints-atoi(buf);
				if(which == STAMINAPOINTS) amount=usernext->staminapoints-atoi(buf);
				if(which == EXPERIENCEPOINTS) amount=usernext->experiencepoints-atoi(buf);
			}
		}

		if(c != '+' && c != '-') {				/* adding/subtracting/setting points */
			strcpy(buf,amountstr);
			amount=atoi(buf);
		}
	
		switch(which) {
			case MAGICPOINTS:
				return(updateuser(currentuser,usernext->name,"",0,0,"",amount,0,0,0,"","",0));

			case STAMINAPOINTS:
				return(updateuser(currentuser,usernext->name,"",0,0,"",0,amount,0,0,"","",0));

			case EXPERIENCEPOINTS:
				return(updateuser(currentuser,usernext->name,"",0,0,"",0,0,amount,0,"","",0));
		}
	}

	usernext=usernext->next;
}

SetLastError(currentuser,UNKNOWN_USER);		/* user not found */
return(-1);
}

/*
* set user level */

int setlevel(user *currentuser,char *u,char *level) {
char c;
int count;
user *usernext;
char *buf[BUF_SIZE];
int newlevel;

if(currentuser->status < WIZARD) {     /* not wizard */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

usernext=users;
while(usernext != NULL) {

	if(regexp(usernext->name,u) == TRUE) {		/* found user */
		c=*level;
	
		if(c == '+') {			/* add points */
			level++;
			strcpy(buf,level);
			newlevel=atoi(buf);

			if(newlevel > 12) {
				SetLastError(currentuser,INVALID_LEVEL);
				return(-1);
			}
	
			 if(newlevel > currentuser->status) {		/* can't set level above own level */
				SetLastError(currentuser,INVALID_LEVEL);
				return(-1);
			 }

			updateuser(currentuser,u,"",0,usernext->status+newlevel,"",0,0,0,0,"","",0);   /* set level */
			return(0);
		}

		if(c == '-') {			/* add points */
			level++;
			strcpy(buf,level);
			newlevel=atoi(buf);

			if(newlevel < 0) {
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}

			return(updateuser(currentuser,u,"",0,usernext->status-newlevel,"",0,0,0,0,"","",0));   /* set level */
		}

		if(c != '+' && c != '-') {
			for(count=1;count<12;count++) {		/* descriptive levels */
				if(strcasecmp(maleusertitles[count],level) == 0) {
					return(updateuser(currentuser,u,"",0,count,"",0,0,0,0,"","",0));   /* set level */
				}
			 }
		}

		newlevel=atoi(level);
		return(updateuser(currentuser,u,"",0,newlevel,"",0,0,0,0,"","",0));   /* set level */
	}		


	usernext=usernext->next;
}

return(0);
}


/* set gender */
int setgender(user *currentuser,char *u,char *gender) {

if(currentuser->status < WIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(strcmp(gender,"male") == 0) {
	return(updateuser(currentuser,u,"",0,0,"",0,0,0,MALE,"","",0));
}

if(strcmp(gender,"female") == 0) {
	return(updateuser(currentuser,u,"",0,0,"",0,0,0,FEMALE,"","",0));
}

SetLastError(currentuser,BAD_GENDER);
return(-1);
}

int loadraces(void) {
race *racenext;
FILE *handle;
int lc;
char *linetokens[10][BUF_SIZE];
char *linebuffer[BUF_SIZE];
int errorcount=0;
char *raceconf[BUF_SIZE];
char *racerel="/config/races.mud";

getcwd(linebuffer,BUF_SIZE);

sprintf(raceconf,"%s%s",linebuffer,racerel);		/* get absolute path of configuration file */

racenext=races;
lc=0;

handle=fopen(raceconf,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",raceconf);
	exit(NOCONFIGFILE);
}

while(!feof(handle)) {
	fgets(linebuffer,BUF_SIZE,handle);		/* get and parse line */

	if((char) *linebuffer == '#')  continue;		/* skip comments */
	if((char) *linebuffer == '\n')  continue;		/* skip newline */

	removenewline(linebuffer);		/* remove newline character */

	lc++;

	tokenize_line(linebuffer,linetokens,":\n");				/* tokenize line */

	if(strcmp(linetokens[0],"begin_race") == 0) {	/* end */

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

		strcpy(racenext->name,linetokens[1]);
		continue;			
	}

	if(strcmp(linetokens[0],"intelligence") == 0) {	/* race points used */
		racenext->intelligence=atoi(linetokens[1]);
		continue;			
	}

	if(strcmp(linetokens[0],"strength") == 0) {
		racenext->strength=atoi(linetokens[1]);
		continue;			
	}

	if(strcmp(linetokens[0],"wisdom") == 0) {
		racenext->wisdom=atoi(linetokens[1]);
		continue;			
	}  

	if(strcmp(linetokens[0],"dexterity") == 0) {
		racenext->dexterity=atoi(linetokens[1]);
		continue;			
	}

	if(strcmp(linetokens[0],"luck") == 0) {
		racenext->luck=atoi(linetokens[1]);
		continue;			
	}

	if(strcmp(linetokens[0],"magic") == 0) {
		racenext->magic=atoi(linetokens[1]);
		continue;			
	}  

	if(strcmp(linetokens[0],"agility") == 0) {
		racenext->agility=atoi(linetokens[1]);
		continue;			
	}

	if(strcmp(linetokens[0],"stamina") == 0) {
		racenext->stamina=atoi(linetokens[1]);
		continue;			
	}

	if(strcmp(linetokens[0],"end") == 0) continue;

	printf("\nmud: %d: uknown configuration option %s in %s\n",lc,linetokens[0],raceconf);		/* unknown configuration option */
	errorcount++;
}


fclose(handle);
return(errorcount);
}


int loadclasses(void) {
class *classnext;
FILE *handle;
int lc;
char *linetokens[10][BUF_SIZE];
char *linebuffer[BUF_SIZE];
int errorcount=0;

getcwd(linebuffer,BUF_SIZE);

sprintf(classconf,"%s/%s",linebuffer,classrel);		/* get absolute path of configuration file */

classnext=classes;
lc=0;

handle=fopen(classconf,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",classconf);

	exit(NOCONFIGFILE);
}

while(!feof(handle)) {
	fgets(linebuffer,BUF_SIZE,handle);		/* get and parse line */

	removenewline(linebuffer);		/* remove newline character */

	lc++;

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


	strcpy(classnext->name,linebuffer);
}

fclose(handle);
return(errorcount);
}

int loadusers(void) {
user *usernext;
FILE *handle;
int lc;
char *linetokens[100][BUF_SIZE];
char *linebuffer[BUF_SIZE];
int errorcount=0;
class *userclass;
class *classlast;
race *racelast;
race *userrace;

getcwd(linebuffer,BUF_SIZE);
sprintf(usersconf,"%s/%s",linebuffer,userrel);		/* get absolute path of configuration file */

handle=fopen(usersconf,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",usersconf);
	exit(NOCONFIGFILE);
}

lc=0;

while(!feof(handle)) {
	fgets(linebuffer,BUF_SIZE,handle);

	if(strlen(linebuffer) < 2) continue;		/* ignore empty lines */

	if(feof(handle)) break;		/* at end */

	removenewline(linebuffer);		/* remove newline character */

	tokenize_line(linebuffer,linetokens,":\n");				/* tokenize line */

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

	strcpy(usernext->name,linetokens[USERNAME]);		/* get details */
	strcpy(usernext->password,linetokens[PASSWORD]);		/* get details */
	usernext->homeroom=atoi(linetokens[HOMEROOM]);
	usernext->status=atoi(linetokens[USERLEVEL]);
	strcpy(usernext->desc,linetokens[DESCRIPTION]);
	usernext->magicpoints=atoi(linetokens[MAGICPOINTS]);
	usernext->staminapoints=atoi(linetokens[STAMINAPOINTS]);
	usernext->experiencepoints=atoi(linetokens[EXPERIENCEPOINTS]);
	usernext->gender=atoi(linetokens[GENDER]);
	usernext->handle=0;
	usernext->flags=atoi(linetokens[USERFLAGS]);
	usernext->next=NULL;

	userrace=races;		/* load race */
	racelast=races;

	while(userrace != NULL) {
		if(strcmp(userrace->name,linetokens[RACE]) == 0) {		/* FOund race */
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
	if(strcmp(userclass->name,linetokens[CLASS]) == 0) {		/* FOund class */
		usernext->userclass=classlast;
		break;
	}

	classlast=userclass;
	userclass=userclass->next;
}

/* update inventory file */

fclose(handle);
return(errorcount);
}

int visible(user *currentuser,char *name,int mode) {
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

int gag(user *currentuser,char *name,int mode) {
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
char *buf[BUF_SIZE];
char *namebuf[BUF_SIZE];
char *linebuffer[10];
int found=FALSE;
user *usernext;

memset(buf,0,BUF_SIZE);

if(!*username) {
	strcpy(namebuf,"*");          /* all users if no username */
}
else
{
	strcpy(namebuf,username);          
}

/*
* show users
*/

usernext=users;

while(usernext != NULL) {
	if((regexp(usernext->name,namebuf) == TRUE) && (usernext->loggedin == TRUE)  && ((usernext->flags & USER_INVISIBLE) == 0)) {			/* found user */
		if(usernext->gender == MALE) {
			sprintf(buf,"%s the %s is in %s (#%d)\r\n",usernext->name,maleusertitles[usernext->status],usernext->roomname,usernext->room);
		}
		else
		{
			sprintf(buf,"%s the %s is in %s (#%d)\r\n",usernext->name,femaleusertitles[usernext->status],usernext->roomname,usernext->room);
		}

		send(currentuser->handle,buf,strlen(buf),0);
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


int go(user *currentuser,int roomnumber) {
room *roomnext;
char *buf[BUF_SIZE];

if(roomnumber == 0) {		/* invalid room */
	SetLastError(currentuser,BAD_DIRECTION);
	return(-1);
}

if(currentuser->room != roomnumber) {	/* send leaving message */
	sprintf(buf,"%s has left\r\n",currentuser->name);
	sendmudmessagetoall(currentuser->room,buf);
}

if(GetRoomFlags(roomnumber) & ROOM_PRIVATE) {
	SetLastError(currentuser,BAD_DIRECTION);
	return(-1);
}	

strcpy(currentuser->roomname,GetRoomName(roomnumber));

currentuser->room=roomnumber;
currentuser->roomptr=GetRoomPointer(roomnumber); 		/* save pointer to current room */

sprintf(buf,"%s has entered\r\n",currentuser->name);
sendmudmessagetoall(currentuser->room,buf);

look(currentuser,"");		/* look at new room */

if(GetRoomFlags(roomnumber) & ROOM_DEAD) {
	killuser(currentuser,currentuser->name);
	return(-1);
}	

return(0);
}

int invisible(user *currentuser,char *username,int which) {
user *usernext;
int found=FALSE;

usernext=users;
while(usernext != NULL) {

	if(regexp(usernext->name,username) == TRUE) {		/* found room */
		if(which == TRUE) {
			usernext->flags |= USER_INVISIBLE;
		}
		else
		{
			usernext->flags &= USER_INVISIBLE;
		}

		found=TRUE;
	}

	usernext=usernext->next;
}

if(found == FALSE) {
	SetLastError(currentuser,UNKNOWN_USER);	/* object not found */
	return(-1);
}

return(0);
}


/*
* move object or player
*/

int moveobject(user *currentuser,char *objectname,int roomnumber) {
char c;
room *destroom;
roomobject *objnext;
roomobject *destobj;
int destination;
user *usernext;
room *currentroom;
char *buf[BUF_SIZE];
int foundroom=FALSE;
int found=FALSE;
CONFIG config;

getconfigurationinformation(&config);

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {      /* not wizard */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(roomnumber > config.lastroom) {			/* can't find room */
	SetLastError(currentuser,BAD_ROOM);
	return(-1);
}

/* move object */
	
objnext=currentroom->roomobjects;

while(objnext != NULL) {
	if(regexp(objnext->name,objectname) == 0 ) {				/* if object matches */
		
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

		if(destroom->roomobjects != NULL) {				/* find end */				
			destobj=destroom->roomobjects;

			while(destobj->next != NULL) destobj=destobj->next; 
			
			destobj->next=calloc(1,sizeof( roomobject));	/* allocate objects */ 

			if(destobj->next == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}

			destobj=destobj->next;
		}
		else
		{						
			destroom->roomobjects=calloc(1,sizeof( roomobject));	/* allocate objects */ 
			destobj=destroom->roomobjects;

			if(destobj == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);
				return(-1);
			}
		}
	
		memcpy(destobj,objnext,sizeof( roomobject));		/* copy object */
		deletething(currentuser,objectname);                                  /* delete object */
	
		found=TRUE;
	}

	objnext=objnext->next;
}


/*
* move player
*/

/* find user */

usernext=users;

while(usernext != NULL) {
	if(regexp(usernext->name,objectname) == 0 && usernext->loggedin == TRUE) {       /* if object matches */

		if(currentuser->status < usernext->status) {  /* can't move user unless wizard or higher level */
			SetLastError(currentuser,NOT_YET);
			return(-1);
		}

		if(go(usernext->handle,roomnumber) == -1) return(-1);

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

int login(int msgsocket,char *uname,char *upass) {
char *encryptedpassword[BUF_SIZE];
char *linetokens[255][255];
FILE *handle;
char *buf[BUF_SIZE];
user *usernext;
user *userlast;
roomobject *objnext;
roomobject *objlast;
int count;
struct sockaddr_in clientip;
socklen_t clientiplen;
char *ipaddress[BUF_SIZE];
char *userinventoryfile[BUF_SIZE];
char *cwd[BUF_SIZE];

clientiplen=sizeof(struct sockaddr_in);			/* get ip address */
getpeername(msgsocket,(struct sockaddr*)&clientip,&clientiplen);

strcpy(ipaddress,inet_ntoa(clientip.sin_addr));

strcpy(encryptedpassword,crypt(upass,uname));

printf("testing login\n");
printf("%s\n",crypt(upass,uname));
printf("%s ***** %s\n",upass,uname);

usernext=users;
userlast=users;

getcwd(cwd,BUF_SIZE);			/* get current directory */

while(usernext != NULL) {
/* check username and password */

	printf("****************\n");
	printf("username=%s %s\n",uname,usernext->name);
	printf("password=%s %s\n",encryptedpassword,usernext->password);

	if(strcmp(uname,usernext->name) == 0 && strcmp(encryptedpassword,usernext->password) == 0) {
		printf("Found user\n");

		strcpy(usernext->ipaddress,ipaddress);	/* get ip address */

		usernext->loggedin=TRUE;		/* user logged in */
		usernext->handle=msgsocket;		/* tcp socket */
		usernext->room=usernext->homeroom;	/* room */
		usernext->roomptr=GetRoomPointer(usernext->homeroom);

		printf("Loading inventory\n");

		/*
		* load user inventory
		*/

		sprintf(userinventoryfile,"%s/config/%s.inv",cwd,usernext->name);		/* get absolute path of user inventory */

		usernext->carryobjects=calloc(1,sizeof( roomobject));		/* allocate objects */
		if(usernext->carryobjects == NULL) {
			PrintError(msgsocket,NO_MEM);
			return(-1);		/* can't allocate */
		}

		objnext=usernext->carryobjects;

		handle=fopen(userinventoryfile,"rb");
		if(handle != NULL) {
			while(!feof(handle)) {
				fgets(buf,BUF_SIZE,handle);	
				if(feof(handle)) break;
	
				removenewline(buf);		/* remove newline character */

				tokenize_line(buf,linetokens,":");		/* tokenize line */
				strcpy(objnext->name,linetokens[OBJECT_NAME]);
				objnext->staminapoints=atoi(linetokens[OBJECT_STAMINAPOINTS]);
				objnext->magicpoints=atoi(linetokens[OBJECT_MAGICPOINTS]);
				objnext->attr=atoi(linetokens[OBJECT_ATTR]);
				objnext->attackpoints=atoi(linetokens[OBJECT_ATTACKPOINTS]);
				objnext->generateprob=atoi(linetokens[OBJECT_GENERATEPROB]);
				strcpy(objnext->desc,linetokens[OBJECT_DESCRIPTION]);
				strcpy(objnext->owner,linetokens[OBJECT_OWNER]);

				objnext->next=calloc(1,sizeof( roomobject));		/* allocate objects */
				if(usernext->carryobjects == NULL) {
					PrintError(msgsocket,NO_MEM);
					return(-1);		/* can't allocate */
				}

				objnext=objnext->next;
			}
		
			objnext->next=NULL;

			fclose(handle);

			printf("Going to home room\n");

			if(go(usernext,usernext->homeroom) == -1) return(-1);
		}

		return(0);
	}


	usernext=usernext->next;
}

return(-1);
}

int createuser(int socket,char *name,char *pass,int gender,char *description,char *racex,char *classx) {
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

updateusersfile();		/* update users file */
return(0);
}


int addnewrace(race *newrace) {
race *next;
race *last;

next=races;
last=next;

if(next != NULL) {
	last=next;
	next=next->next;
}

last->next=calloc(1,sizeof(race));		/*  add new */
if(last->next == NULL) return(-1);

memcpy(last->next,newrace,sizeof(race));
return(0);
}

int addnewclass(user *currentuser,class *newclass) {
class *next;
class *last;

if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);  
	return(-1);
}

next=classes;
last=next;

if(next != NULL) {
	last=next;
	next=next->next;
}

last->next=calloc(1,sizeof(class));		/*  add new */
if(last->next == NULL) return(-1);

memcpy(last->next,newclass,sizeof(class));
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
return(maleusertitles[level]);
}

char *GetPointerToFemaleTitles(int level) {
return(maleusertitles[level]);
}

void AttackUser(int roomnumber,int roommonster) {
user *usernext=users;
int sta;
char *buf[BUF_SIZE];

while(usernext != NULL) {
	if(usernext->room == roomnumber) {		/* user is in room */
		sta=rand() % (GetRoomMonsterEvil(roomnumber,roommonster) + 1) - 0;		/* random damage */

		sprintf(buf,"%s attacks %s causing %d points of damage\r\n",GetRoomMonsterName(roomnumber,roommonster),usernext->name,sta);
		sendmudmessagetoall(roomnumber,buf);

		usernext->staminapoints=usernext->staminapoints-sta;

		updateuser(usernext,usernext->name,"",0,0,"",0,usernext->staminapoints,0,0,"","",0);

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

