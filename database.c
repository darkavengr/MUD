/* database function */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define _XOPEN_SOURCE
#define _GNU_SOURCE

#include <fcntl.h>
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
#include <time.h>

#include "bool.h"
#include "directions.h"
#include "errors.h"
#include "user.h"
#include "room.h"
#include "config.h"

char *dbconf[BUF_SIZE];
char *dbrel="/config/database.mud";                                          /* configuration files */
int databaseupdated;

char *directions[]={ "north","south","east","west","northeast","northwest","southeast","southwest","up","down" };

char *createmessage= { "A room has been created to the north\r\n","A room has been created to the south\r\n",\
						 "A room has been created to the east\r\n","A room has been created to the west\r\n",\
						 "A room has been created to the northeast\r\n","A room has been created to the northwest\r\n",\
						 "A room has been created to the southeast\r\n","A room has been created to the southwest\r\n",\
						 "A room has been created above you\r\n","A room has been created below you\r\n" };
char *resetconf[BUF_SIZE];
char *resetrel="/config/reset.mud";
char *roommsg="\r\nIn the room there is: ";
char *roomexits[]={ "North ","South ","East ","West ","Northeast ","Northwest ","Southeast ","Southwest ","Up ","Down " };

room *rooms=NULL;

/* update database file */

int updatedatabase(void) {
FILE *handle;
char *newname[BUF_SIZE];
char *buf[BUF_SIZE];
time_t rawtime;
struct tm *timeinfo;
roomobject *roomobjects;
int count;
CONFIG config;
char *currentdirectory[BUF_SIZE];
char *backupfilename[BUF_SIZE];

getconfigurationinformation(&config);

time(&rawtime);
timeinfo=localtime(&rawtime);

/* if configured to backup, copy database to backup file */

if(config.databasebackup == TRUE) {
	getcwd(currentdirectory,BUF_SIZE);		/* get current directory */

	strftime(newname,BUF_SIZE,"/config/database-%H-%M-%S-%d-%e.sav",timeinfo);	/* get backup name */

	sprintf(backupfilename,"%s/%s",currentdirectory,newname);			/* get path of backup */

	copyfile(dbconf,buf);		/* backup database */
}

	handle=fopen(dbconf,"w");
	if(handle == NULL) return(-1);	/* can't open file */

	/*
	* for each room output to file
	*/

	for(count=0;count<config.roomcount+1;count++) {
		if(rooms[count].room > 0) {
			fprintf(handle,"begin_room:%d\n",rooms[count].room);
			fprintf(handle,"name:%s\n",rooms[count].name);
			fprintf(handle,"owner:%s\n",rooms[count].owner);
			fprintf(handle,"attr:%d\n",rooms[count].attr);
			fprintf(handle,"description:%s\n",rooms[count].desc);
			fprintf(handle,"north:%d\n",rooms[count].exits[NORTH]);
			fprintf(handle,"south:%d\n",rooms[count].exits[SOUTH]);
			fprintf(handle,"east:%d\n",rooms[count].exits[EAST]);
			fprintf(handle,"west:%d\n",rooms[count].exits[WEST]);
			fprintf(handle,"northeast:%d\n",rooms[count].exits[NORTHEAST]);
			fprintf(handle,"southeast:%d\n",rooms[count].exits[SOUTHEAST]);
			fprintf(handle,"northwest:%d\n",rooms[count].exits[NORTHWEST]);
			fprintf(handle,"northeast:%d\n",rooms[count].exits[NORTHEAST]);
			fprintf(handle,"up:%d\n",rooms[count].exits[UP]);
			fprintf(handle,"down:%d\n",rooms[count].exits[DOWN]);

			/*
			* output objects
			*/

			roomobjects=rooms[count].roomobjects;

			while(roomobjects != NULL) {
				if(roomobjects->attr & OBJECT_TEMPORARY) {
					roomobjects=roomobjects->next;    
					continue;
				}

				if(*roomobjects->name) fprintf(handle,"object:%s:%d:%d:%d:%d:%s:%s:%d\n",roomobjects->name,roomobjects->staminapoints,\
												 roomobjects->magicpoints,roomobjects->attackpoints,\
												 roomobjects->generateprob,roomobjects->desc,\
												 roomobjects->owner,roomobjects->attr);
				
				roomobjects=roomobjects->next;
			}

			fprintf(handle,"end\n");
		}
	}

fclose(handle);
return(0);
}

int loaddatabase(void) {
FILE *handle;
int lc=0;
roomobject *roomobject;
char *linebuffer[BUF_SIZE];
char *b;
char c;
char *linetokens[19][BUF_SIZE];
int count;
int errorcount=0;
int countx;
CONFIG config;
char *currentdirectory[BUF_SIZE];

getconfigurationinformation(&config);

/* get path of database */

getcwd(currentdirectory,BUF_SIZE);
sprintf(dbconf,"%s/%s",currentdirectory,dbrel);

handle=fopen(dbconf,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",dbconf);
	exit(NOCONFIGFILE);
}

config.roomcount=0;

while(!feof(handle)) {
		fgets(linebuffer,BUF_SIZE,handle);				/*' get and parse line */
		
		tokenize_line(linebuffer,linetokens,":\n");				/* tokenize line */
		if(strcmp(linetokens[0],"begin_room") == 0) {
			config.roomcount++;
			updateconfigurationinformation(&config);

		}

}

fseek(handle,0,SEEK_SET);

config.databasememorysize=sizeof(room)*config.roomcount;

rooms=calloc(config.roomcount,config.databasememorysize);	/* allocate buffer */
if(rooms == NULL) {
	perror("mud:");
	exit(NOMEM);
}

count=0;

	while(!feof(handle)) {
		fgets(linebuffer,BUF_SIZE,handle);				/*' get and parse line */
		b=linebuffer;
		c=*b;
		if(c == '#')  continue;		/* skip comments */
		if(c == '\n')  continue;		/* skip newline */
	
		removenewline(linebuffer);		/* remove newline character */

		tokenize_line(linebuffer,linetokens,":\n");				/* tokenize line */

		lc++;

		if(strcmp(linetokens[0],"begin_room") == 0) {		/* room name */
			count=atoi(linetokens[1]);
		
			if(count > config.roomcount) {
				printf("\nmud: %d: room number is > number of rooms in %s\r\n",lc,dbconf); /* unknown configuration option */
				errorcount++;
				exit(1);
			}

			memset(&rooms[count],0,sizeof(room));

			rooms[count].room=atoi(linetokens[1]);
			continue;
		}

		if(strcmp(linetokens[0],"name") == 0) {		/* room name */
			strcpy(rooms[count].name,linetokens[1]);
			continue;
		}

		if(strcmp(linetokens[0],"owner") == 0) {		/* owner */
			strcpy(rooms[count].owner,linetokens[1]);
			continue;
		}

		if(strcmp(linetokens[0],"attr") == 0) {		/* attributes */
			rooms[count].attr=atoi(linetokens[1]);
			continue;
		}

		if(strcmp(linetokens[0],"description") == 0) {		/* description */
			strcpy(rooms[count].desc,linetokens[1]);
			continue;
		}


		if(strcmp(linetokens[0],"name") == 0) {
			strcpy(rooms[count].name,linetokens[1]);
			continue;
		}

		for(countx=0;countx<12;countx++) {
			if(strcmp(linetokens[0],directions[countx]) == 0) {		/* room exits */
				
				if(atoi(linetokens[1])  > config.roomcount) {
					printf("\nmud: %d: room number is > number of rooms in %s\r\n",lc,dbconf); /* unknown configuration option */
					errorcount++;
					exit(1);
				}

				rooms[count].exits[countx]=atoi(linetokens[1]);
				break;
			}

			continue;
		}

		if(countx < 12) continue;		/* found room */

		if(strcmp(linetokens[0],"object") == 0) {		/* room object */
			if(rooms[count].roomobjects == NULL) {		/* first object */
				rooms[count].roomobjects=calloc(1,sizeof(roomobject));		/* add link to end */
				if(rooms[count].roomobjects == NULL) {		/* can't allocate */
					perror("mud:");
					exit(NOMEM);
				}

				roomobject=rooms[count].roomobjects;
				rooms[count].roomobjects_last=rooms[count].roomobjects;
			}
			else
			{
				rooms[count].roomobjects_last->next=calloc(1,sizeof(roomobject));		/* add link to end */
				if(rooms[count].roomobjects_last->next == NULL) {		/* can't allocate */
					perror("mud:");
					exit(NOMEM);
				}

				rooms[count].roomobjects_last=rooms[count].roomobjects_last->next;
				roomobject=rooms[count].roomobjects_last;
			}

			strcpy(roomobject->name,linetokens[OBJECT_NAME]);		/* get details */
			roomobject->attr=atoi(linetokens[OBJECT_ATTR]);
			strcpy(roomobject->desc,linetokens[OBJECT_DESCRIPTION]);		
			roomobject->attackpoints=atoi(linetokens[OBJECT_ATTACKPOINTS]);		
			roomobject->staminapoints=atoi(linetokens[OBJECT_GENERATEPROB]);
			roomobject->magicpoints=atoi(linetokens[OBJECT_MAGICPOINTS]);   
			roomobject->next=NULL;
			continue;
		}

		if(strcmp(linetokens[0],"end") == 0) {
			continue;
		}

		if(strcmp(linetokens[0],"#") == 0) continue;			/* comment */

		printf("\nmud: %d: unknown configuration option %s in %s\n",lc,linetokens[0],dbconf);		/* unknown configuration option */
		errorcount++;
		continue;
	}

fclose(handle);
return(errorcount);
}

int setexit(user *currentuser,int whichroom,int direction,int exit) {
CONFIG config;

getconfigurationinformation(&config);

if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(direction > 11) {
	SetLastError(currentuser,INVALID_EXIT);
	return(-1);
}

if(direction > config.roomcount) {
	SetLastError(currentuser,BAD_ROOM);  
	return(-1);
}

rooms[whichroom].exits[direction]=exit;
return(0);
}

/* create room */

int createroom(user *currentuser,char *roomdirection) {
room *roomnext;
room *currentroom;
room temproom;
char *buf[BUF_SIZE];
int roomdir;
CONFIG config;

getconfigurationinformation(&config);

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {		/* must be wizard or higher level to create room */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/*
	deny if not owner and not allowed to create exit
*/

if(currentuser->status < ARCHWIZARD) {
	if((strcmp(rooms[currentuser->room].owner,currentuser->name) !=0) && (rooms[currentuser->room].attr  & ROOM_EXIT_PUBLIC) == 0 && currentuser->status < ARCHWIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	/*
	* deny permission if not owner not allowed to create objects in room
	*/

	if((strcmp(rooms[currentuser->room].owner,currentuser->name) == 0) && (rooms[currentuser->room].attr  & ROOM_EXIT_OWNER) == 0 && currentuser->status < ARCHWIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}
}


/*
	* if the room is in argument, create it */

if(*roomdirection) {			/* room spcified */
	for(roomdir=0;roomdir != 11;roomdir++) {
			if(strcmp(roomdirection,directions[roomdir]) == 0) break;	/* found */
	}
}
else
{
	for(roomdir=0;roomdir != 11;roomdir++) {
			if(rooms[currentuser->room].exits[roomdir] == 0) break;	/* found */
	}
}

if(roomdir > 11) { 
	SetLastError(currentuser,BAD_DIRECTION);  
	return(-1);
}

config.databasememorysize += sizeof(room);

roomnext=realloc(rooms,config.databasememorysize);		/* resize database */
if(roomnext == NULL) {
	SetLastError(currentuser,CANT_CREATE_ROOM);  
	return(-1);
}

config.roomcount++;

memset(&rooms[config.roomcount],0,sizeof(room));		/* clear room */
strcpy(rooms[config.roomcount].name,"Empty room\r\r\n");		/* create room info */
strcpy(rooms[config.roomcount].owner,currentuser->name);
strcpy(rooms[config.roomcount].desc,"Empty room, you can describe it using desc here <description>\r\r\n");
rooms[config.roomcount].attr=ROOM_CREATE_OWNER+ROOM_EXIT_OWNER+ROOM_RENAME_OWNER;
rooms[config.roomcount].room=++config.lastroom;

currentroom->exits[roomdir]=config.roomcount;

updateconfigurationinformation(&config);

send(currentuser->handle,createmessage[roomdir],strlen(roomdir),0);

databaseupdated=TRUE;                  /* update database flag */
return(0);
}

/* set an object, room or user's attributes */

int setobjectattributes(user *currentuser,char *object,char *attr) {
char *cb[109][255];
int cpc;
roomobject *objnext;
room *roomnext;
user *usernext;
room *currentroom;
int count;
int found=FALSE;	
char *b;
char *buf[BUF_SIZE];
CONFIG config;

getconfigurationinformation(&config);

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {            /* need to be wizard or higher level to change attributes */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

cpc=tokenize_line(attr,cb," \009");			/* tokenize line */

objnext=currentroom->roomobjects;		/* point to objects */

while(objnext != NULL) {

	if(regexp(objnext->name,object) == TRUE) {		/* found object */
		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(objnext->owner,currentuser->name) == 0) && (objnext->attr & OBJECT_MOVEABLE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			if((strcmp(objnext->owner,currentuser->name) == 0) && (objnext->attr & OBJECT_MOVEABLE_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		for(count=0;count<cpc;count++) {
			b=cb[count];		/* point to argument */

			if(*b == '+') {	/* setting attribute */

				while(*b != 0) {
					switch(*b++) {
						case 'd':
							objnext->attr |= OBJECT_DELETE_OWNER;
					  		 break;

						case 'D':
					   		objnext->attr |= OBJECT_DELETE_PUBLIC;
					   		break;

						case 'm':
					   		objnext->attr |= OBJECT_MOVEABLE_OWNER;
					   		break;
		
						case 'M':
							objnext->attr |= OBJECT_MOVEABLE_PUBLIC;
							break;
		
						case 'p':
							objnext->attr |= OBJECT_PICKUP_OWNER;
							break;

						case 'P':
					   		objnext->attr |= OBJECT_PICKUP_PUBLIC;
					   		break;
		
						case 'r':
							objnext->attr |= OBJECT_RENAME_OWNER;
							break;
		
						case 'R':
							objnext->attr |= OBJECT_RENAME_PUBLIC;
							break;
		
					 	case 't':
					   		objnext->attr |= OBJECT_TEMPORARY;
					   		break;
		
						default:
							sprintf(buf,"Unknown attribute %c\r\n",*b);
					 
						}
			}

			if(*b == '-') {
				while(*b != 0) {
					switch(*b++) {
						case 'd':
							objnext->attr &= OBJECT_DELETE_OWNER;
							break;

						case 'D':
							objnext->attr &= OBJECT_DELETE_PUBLIC;
							break;

					  	case 'm':
					   		objnext->attr &= OBJECT_MOVEABLE_OWNER;
					   		break;
		
					  	case 'M':
					   		objnext->attr &= OBJECT_MOVEABLE_PUBLIC;
					   		break;
		
					 	case 'p':
					   		objnext->attr &= OBJECT_PICKUP_OWNER;
					   		break;

					 	case 'P':
					   		objnext->attr &= OBJECT_PICKUP_PUBLIC;
					   		break;
		
					 	case 'r':
							objnext->attr &= OBJECT_RENAME_OWNER;
							break;
		
					 	case 'R':
					   		objnext->attr &= OBJECT_RENAME_PUBLIC;
					   		break;
		
					 	case 't':
					   		objnext->attr &= OBJECT_TEMPORARY;
					   		break;
		
						default:
					  		sprintf(buf,"Unknown attribute %c\r\n",*b);
							send(currentuser->handle,buf,strlen(buf),0);
						}
					}
				}
			} 

			found=TRUE;
		}


	objnext=objnext->next;
}


for(count=0;count<config.roomcount;count++) {
	if(regexp(rooms[count].name,object) == TRUE) {		/* found room */
		b=cb[count];

		if(*b == '+') {	/* setting attribute */
			b++;

			while(*b != 0) {

				switch(*b++) {
				  	case 'c':
						rooms[count].attr |= ROOM_CREATE_OWNER;
						break;

					case 'C':
						rooms[count].attr |= ROOM_CREATE_PUBLIC;
						break;

					case 'e':
						rooms[count].attr |= ROOM_EXIT_OWNER;
						break;

					case 'E':
						rooms[count].attr |= ROOM_CREATE_PUBLIC;
						break;
	
					case 'r':
						rooms[count].attr |= ROOM_RENAME_OWNER;
						break;

					case 'R':
						rooms[count].attr |= ROOM_RENAME_PUBLIC;
						break;

					case 'h':
						rooms[count].attr |= ROOM_HAVEN;
						break;

					case 'p':
						rooms[count].attr |= ROOM_PRIVATE;
						break;

					default:
					    sprintf(buf,"Unknown attribute %c\r\n",*b);
					    send(currentuser->handle,buf,strlen(buf),0);
					}
				}
			}
		}

		if(*b == '-') {
			while(*b != 0) {	
				switch(*b++) {
				 	case 'c':
						rooms[count].attr &= ROOM_CREATE_OWNER;
						break;

				 	case 'C':
						rooms[count].attr &= ROOM_CREATE_PUBLIC;
						break;

					case 'e':
						rooms[count].attr &= ROOM_EXIT_OWNER;
						break;

				 	case 'E':
						rooms[count].attr &= ROOM_EXIT_PUBLIC;
						break;
	
				 	case 'r':
						rooms[count].attr &= ROOM_RENAME_OWNER;
						break;

					 case 'R':
						rooms[count].attr &= ROOM_RENAME_PUBLIC;
						break;

				 	case 'h':
						rooms[count].attr &= ROOM_HAVEN;
						break;

				 	case 'p':
						rooms[count].attr &= ROOM_PRIVATE;
						break;

					default:
						sprintf(buf,"Unknown attribute %c\r\n",*b);
						send(currentuser->handle,buf,strlen(buf),0);
					}

				}
			}

		found=TRUE;
	}
}


if(found == FALSE)  {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  
	return(-1);
}

return(0);
}


/* set owner of room or object */

int setowner(user *currentuser,char *objectname,char *n) {
room *roomnext;
roomobject *objnext;
room *currentroom;
int count;
CONFIG config;

getconfigurationinformation(&config);

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}
	
if(currentuser->status <ARCHWIZARD) {

	if(strcmp(n,currentuser->name) == 0) {                    /* unless archwizard, or higher, must be object's owner to modify */
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	} 
}

/*
* find object
*/

objnext=currentroom->roomobjects;
while(objnext != NULL) {

	if(regexp(objnext->name,objectname) == TRUE) { 			/* if object found */
		strcpy(objnext->owner,n);
		databaseupdated=TRUE;

		return(0);
	}

	objnext=objnext->next;
}

/*
* not object, find room to change owner
*/

for(count=0;count<config.roomcount;count++) {
	if(rooms[count].room == atoi(objectname)) {		/* if object found */

		if(currentuser->status < ARCHWIZARD && strcmp(rooms[count].owner,currentuser->name) != 0) {	/* permission denied */
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
		}

		strcpy(rooms[count].owner,n);   

		databaseupdated=TRUE;       			/* update database */
		return(0);
	}

}
		
SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(0);
}

/* copy object */

int copyobject(user *currentuser,char *o,int l) {
room *destroom;
roomobject *objnext;
roomobject *destobj;
int destination;
user *usernext;
room *currentroom;
char *buf[BUF_SIZE];
int foundroom=FALSE;
int found=FALSE;
int count;
CONFIG config;

getconfigurationinformation(&config);

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {      /* not wizard */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(config.roomcount > currentroom->room) {			/* can't find room */
	SetLastError(currentuser,BAD_ROOM);  
	return(-1);
}

/* move object */
	
objnext=currentroom->roomobjects;

while(objnext != NULL) {
	if(regexp(objnext->name,o) == TRUE) {				/* if object matches */
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

		/* copy object */
		if(rooms[l].roomobjects == NULL) {
			rooms[l].roomobjects=calloc(1,sizeof( roomobject));	/* allocate objects */ 
			if(rooms[l].roomobjects == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);  
				return(-1);
			}

			rooms[l].roomobjects_last=rooms[l].roomobjects;
		}
		else
		{
			rooms[l].roomobjects_last->next=calloc(1,sizeof( roomobject));	/* allocate objects */ 
			if(rooms[l].roomobjects == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);  
				return(-1);
			}

			rooms[l].roomobjects_last=rooms[l].roomobjects_last->next;
		}

		memcpy(rooms[l].roomobjects_last,objnext,sizeof(roomobject));		/* copy object */
		
		found=TRUE;
	}

	objnext=objnext->next;
}

/*
* move player
*/

usernext=GetUserPointerByName(o);		/* find user */

if(usernext != NULL) {			/* found user */
	go(usernext,l);
	found=TRUE;
}

/* copy room */

if((char) *o == '#') {
	o++;
	destination=atoi(o);

	memcpy(&rooms[destination],&rooms[l],sizeof(room));
	found=TRUE;
}

if(found == FALSE)  {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  
	return(-1);
}

return(0);
}

/* generate objects */

int resetobjects(void) {
FILE *handle;
roomobject *objnext;
roomobject *objlast;
char *tokens[BUF_SIZE][BUF_SIZE];
int NumberOfObjectsToGenerate;
int randnumber;
char *buf=NULL;
char *bufptr;
int objectcount=0;
int gencount;
int count=0;
int roomobjectcount;
CONFIG config;
char *currentdirectory[BUF_SIZE];

getconfigurationinformation(&config);

getcwd(currentdirectory,BUF_SIZE);
sprintf(resetconf,"%s/%s",currentdirectory,resetrel);

handle=fopen(resetconf,"rb");
if(handle == NULL) return(-1);		/* can't open configution file */

/*
* load objects into array */

while(!feof(handle)) {
	if(buf == NULL) {
		buf=malloc(BUF_SIZE);
		if(buf == NULL) return(-1);
	}
	else
	{
		objectcount++;

		buf=realloc(buf,BUF_SIZE*(objectcount+1));
		if(buf == NULL) return(-1);
	}

	fgets(&buf[objectcount*BUF_SIZE],BUF_SIZE,handle);                                      /* get line into array */
}

fclose(handle);


srand(time(NULL));

/*
* for each room, generate objects depending on probability
*/

for(count=0;count<config.roomcount;count++) {

	if((rooms[count].attr & ROOM_HAVEN) == 0) {          /* not haven */
		NumberOfObjectsToGenerate=rand() % (config.roomobjectnumber + 1) - 0;

		for(roomobjectcount=1;roomobjectcount<NumberOfObjectsToGenerate;roomobjectcount++) {   
			randnumber=rand() % (objectcount + 1) - 0;
			
			tokenize_line(&buf[randnumber*BUF_SIZE],tokens,":");		/* tokenize line */

			if(rooms[roomobjectcount].roomobjects == NULL) {	/* no objects in room */
				rooms[roomobjectcount].roomobjects=calloc(1,sizeof(roomobject));		/* add link to end */
				if(rooms[roomobjectcount].roomobjects == NULL) continue;

				rooms[roomobjectcount].roomobjects_last=rooms[roomobjectcount].roomobjects;
			}
        		else
        		{
				rooms[roomobjectcount].roomobjects_last->next=calloc(1,sizeof(roomobject));		/* add link to end */
				rooms[roomobjectcount].roomobjects_last=rooms[roomobjectcount].roomobjects_last->next;
			}

	
			/* add object to room */

			rooms[roomobjectcount].roomobjects_last->room=count;
			strcpy(rooms[roomobjectcount].roomobjects_last->name,tokens[OBJECT_NAME]);
			strcpy(rooms[roomobjectcount].roomobjects_last->owner,"nobody");
			strcpy(rooms[roomobjectcount].roomobjects_last->desc,tokens[OBJECT_DESCRIPTION]);
			rooms[roomobjectcount].roomobjects_last->attackpoints=atoi(tokens[OBJECT_ATTACKPOINTS]);
			rooms[roomobjectcount].roomobjects_last->generateprob=atoi(tokens[OBJECT_GENERATEPROB]);
			rooms[roomobjectcount].roomobjects_last->staminapoints=atoi(tokens[OBJECT_STAMINAPOINTS]);
			rooms[roomobjectcount].roomobjects_last->magicpoints=atoi(tokens[OBJECT_MAGICPOINTS]);
			rooms[roomobjectcount].roomobjects_last->attr=OBJECT_TEMPORARY;
		}   
	}

}

free(buf);
return(0);
}

int pickup(user *currentuser,char *o) {
roomobject *obj;
char *buf[BUF_SIZE];

obj=currentuser->carryobjects;

while(obj != NULL) {
	if(regexp(obj->name,o) == TRUE) {	/* already picked up object */
		sprintf(buf,"You already have a %s\r\n",obj->name);
		send(currentuser->handle,buf,strlen(buf),0);

		return(-1);
	}

	obj=obj->next;
}

/* add object to inventory */

if(currentuser->carryobjects == NULL) {
	currentuser->carryobjects=calloc(1,sizeof(roomobject));		/* add link to end */
	if(currentuser->carryobjects == NULL) {		/* can't allocate */
		sprintf(buf,"Can't pick up object %s\r\n",o);
		send(currentuser->handle,buf,strlen(buf),0);
		return(-1);
	}

	currentuser->carryobjects_last=currentuser->carryobjects;
}
else
{  
	currentuser->carryobjects_last->next=calloc(1,sizeof(roomobject));		/* add link to end */
	if(currentuser->carryobjects_last->next == NULL) {		/* can't allocate */
		sprintf(buf,"Can't pick up object %s\r\n",o);
		send(currentuser->handle,buf,strlen(buf),0);

		return(-1);
	}

	currentuser->carryobjects_last=currentuser->carryobjects_last->next;

	strcpy(currentuser->carryobjects_last->name,o);		/* add item */

	if(obj->magicpoints > 0) {
		sprintf(buf,"You have gained %d magic points!\r\n",obj->magicpoints);
		send(currentuser->handle,buf,strlen(buf),0);
	}

	if(obj->staminapoints > 0) {
		sprintf(buf,"You have gained %d stamina points!\r\n",obj->staminapoints);
		send(currentuser->handle,buf,strlen(buf),0);
	}  

	obj->magicpoints=0;
	obj->staminapoints=0;
	obj->next=NULL;
}

return(0);
}


int drop(user *currentuser,char *o) {
roomobject *obj;
roomobject *objlast;
room *currentroom;
roomobject *roomobj;
char *buf[BUF_SIZE];
int found=FALSE;

/* check permissions */

currentroom=currentuser->roomptr;

if(currentuser->status < ARCHWIZARD) {

	if(strcmp(currentroom->owner,currentuser->name) == 0) {

		if((currentroom->attr & ROOM_CREATE_OWNER) == 0) {
			SetLastError(currentuser,CANT_CREATE_OBJECTS_HERE);  
			return(-1);
		}
		else
		{
			if((currentroom->attr & ROOM_CREATE_PUBLIC) == 0) {
				SetLastError(currentuser,CANT_CREATE_OBJECTS_HERE);  
				return(-1);
			}
		}
	}
}

obj=currentuser->carryobjects;		/* point to carried objects */

objlast=obj;

while(obj != NULL) {
	
	if(regexp(obj->name,o) == TRUE) {	/* found object */
		/* add to list of objects in room */

		if(currentroom->roomobjects == NULL) {					       
			currentroom->roomobjects=calloc(1,sizeof(roomobject));	/* allocate objects */
			if(currentroom->roomobjects == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);  
				return(-1);
			}

			currentroom->roomobjects_last=currentroom->roomobjects;
		}
		else
		{
			currentroom->roomobjects_last=calloc(1,sizeof(roomobject));	/* allocate objects */
			if(currentroom->roomobjects_last == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);  
				return(-1);
			}
		}

		currentroom->roomobjects_last->next=currentroom->roomobjects_last;
	}

	memcpy(roomobj,obj,sizeof(roomobject));	/* copy object */

	if(obj == currentuser->carryobjects) {		/* first object */
		obj=obj->next;

		free(currentuser->carryobjects);   
		currentuser->carryobjects=obj;
		found=TRUE;
	}

	if(obj->next == NULL) {		/* last object */
		found=TRUE;
	      	free(obj);	
	}

	if(obj != currentuser->carryobjects && obj->next != NULL) {      
		objlast->next=obj->next;	/* skip over over object */
		free(obj);
	}

	found=TRUE;

	objlast=obj;
	obj=obj->next;
}

if(found == FALSE) {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  	/* not found */
	return(-1);
}

SetLastError(currentuser,NO_ERROR);
return(0);
}


/* create object */

int createobject(user *currentuser,char *objname) {
roomobject *objnext;
roomobject *objlast;
room *currentroom;
char *buf[BUF_SIZE];

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {         /* can't create object unless wizard level or above */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/*
* deny permission if not archwizard and owner doesn't allow others to create objects in room
*/

if(currentuser->status < ARCHWIZARD) {
	if((strcmp(currentroom->owner,currentuser->name) !=0) && (currentroom->attr  & ROOM_CREATE_PUBLIC) == 0 && currentuser->status < ARCHWIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

/*
	* deny permission if not archwizard and owner not allowed to create objects in room
	*/

	if((strcmp(currentroom->owner,currentuser->name) == 0) && (currentroom->attr  & ROOM_CREATE_OWNER) == 0 && currentuser->status < ARCHWIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}
}

/*
* check if object already exists and display error message if it does
*/

objnext=currentroom->roomobjects;

while(objnext != NULL) {
	
	if(strcmp(objname,objnext->name) == 0) {
		SetLastError(currentuser,OBJECT_EXISTS);  
		return(-1);
	}

	objlast=objnext;
	objnext=objnext->next;
}
	
if(currentroom->roomobjects == NULL) {		/* no objects */
	currentroom->roomobjects=calloc(1,sizeof(roomobject));		/* add link to end */
	if(currentroom->roomobjects == NULL) {		/* can't allocate */
		SetLastError(currentuser,NO_MEM);  
		return(-1);
	}

	objnext=currentroom->roomobjects;
}
else
{
	objnext=currentroom->roomobjects_last;
}


objnext->next=calloc(1,sizeof(roomobject));
if(objnext == NULL) {		/* can't allocate */
	SetLastError(currentuser,NO_MEM);  
	return(-1);
}

objnext=objnext->next;

/*
* add object details at end
*/

strcpy(objnext->name,objname);
objnext->attr=OBJECT_DELETE_OWNER+OBJECT_MOVEABLE_OWNER+OBJECT_RENAME_OWNER;
objnext->magicpoints=0;
strcpy(objnext->owner,currentuser->name);
strcpy(objnext->desc,"No description");
objnext->next=NULL;

databaseupdated=TRUE;                                     /* mark database as updated */
return(0);
}

/* delete object */

int deletething(user *currentuser,char *o) {
roomobject *objnext;
roomobject *objlast;
room *currentroom;

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/*
* find object
*/

objnext=currentroom->roomobjects;
objlast=objnext;

while(objnext != NULL) {
	objlast=objnext;

	if(regexp(objnext->name,o) == TRUE) {		/* found */

		/*
		* can't delete if not owner and OBJECT_DELETE_PUBLIC attribute not set
		*/


		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(objnext->owner,currentuser->name) != 0) && (objnext->attr & OBJECT_DELETE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			/*
			* if object owner and OBJECT_MOVEABLE_OWNER attribute not set, display error message
			*/

			if((strcmp(objnext->owner,currentuser->name) == 0) && (objnext->attr & OBJECT_DELETE_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		/*
		*
		* remove object from room
		*/


		if(objnext == currentroom->roomobjects) {		/* first object */
			objnext=objnext->next;

			free(currentroom->roomobjects);   
			currentroom->roomobjects=objnext;
		}
					    
		if(objnext != currentroom->roomobjects && objnext->next == NULL) {		/* last object */
			free(objnext);	
			
			currentroom->roomobjects_last=objlast->next;	/* new end of list */
		}

		if(objnext != currentroom->roomobjects && objnext->next != NULL) {      
			objlast->next=objnext->next;	/* skip over over object */
			free(objnext);
		}

		databaseupdated=TRUE;                                       /* database update flag */
		return(0);
	}

	objlast=objnext;
	objnext=objnext->next;
}

SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}


int renameobject(user *currentuser,char *o,char *n) {
user *usernext;
roomobject *objnext;
room *currentroom;

currentroom=currentuser->roomptr;

if(currentuser->status < WIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/* renaming user */

usernext=GetUserPointerByName(o);		/* find user */
if(usernext != NULL) {			/* found user */
	strcpy(usernext,n);	/* set username */

	if(updateuser(currentuser,currentuser->name,"",0,0,"",0,currentuser->staminapoints,0,0,"","",0) == -1) return(-1);
}



/* renaming object */
objnext=currentroom->roomobjects;

	while(objnext->next != NULL) {

		if(regexp(objnext->name,o) == TRUE) {		/* found user */
			/* check permissions */

			if(currentuser->status < ARCHWIZARD) {

			/* is owner but can't rename */

				if(strcmp(objnext->owner,currentuser->next) == 0 && (objnext->attr & OBJECT_RENAME_OWNER) == 0) {
					SetLastError(currentuser,ACCESS_DENIED);
					return(-1);
				}

			/* not owner and can't rename */

			if(strcmp(objnext->owner,currentuser->next) != 0 && (objnext->attr & OBJECT_RENAME_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		strcpy(objnext->name,n);		/* rename object */
		return(0);
	}

	objnext=objnext->next;
}

SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}

/* look at object, user or room */

int look(user *currentuser,char *n) {
char *buf[BUF_SIZE];
char *linebuffer[10];
roomobject *objectnext;
user *usernext;
room *currentroom;
int found=FALSE;
int count;

currentroom=currentuser->roomptr;
	
/* no name, so look at current room */

printf("n=%s\n",n);

printf("handle=%d\n",currentuser->handle);

if(!*n) {				/* display name */
	sprintf(buf,"\r\n#%d %s\r\n",currentroom->room,currentroom->name);

	send(currentuser->handle,buf,strlen(buf),0);  
	send(currentuser->handle,currentroom->desc,strlen(currentroom->desc),0);  

	send(currentuser->handle,"\r\nExits: ",8,0);  		/* display exits */

	for(count=0;count<11;count++) {
		if(currentroom->exits[count] != 0) send(currentuser->handle,roomexits[count],strlen(roomexits[count]),0);
	}

	send(currentuser->handle,"\r\n",2,0);

	if(currentroom->roomobjects != NULL) {		/* display objects */
		send(currentuser->handle,"\r\n",2,0);
		send(currentuser->handle,roommsg,strlen(roommsg),0);
		objectnext=currentroom->roomobjects;

		while(objectnext != NULL) {
			send(currentuser->handle,objectnext->name,strlen(objectnext->name),0);
			send(currentuser->handle," ",1,0);

			objectnext=objectnext->next;
		}

		send(currentuser->handle,"\r\n",2,0);
	}

/*
* display monsters in room
*
*/

	send(currentuser->handle,"\r\n",2,0);

	for(count=0;count<currentroom->monstercount;count++) {
		sprintf(buf,"a %s is here\r\n",currentroom->roommonsters[count].name);
		send(currentuser->handle,buf,strlen(buf),0);
	}

/*
* display users in room
*/

usernext=FindFirstUser();		/* find first user */

while(usernext != NULL) {
	if((regexp(usernext->name,n) == TRUE) && (usernext->loggedin == TRUE) && (usernext->room == currentuser->room)) {
		if(usernext->gender == MALE) {
			sprintf(buf,"%s the %s is here\r\n",usernext->name,GetPointerToMaleTitles(usernext->status));
		}
		else
		{
			sprintf(buf,"%s the %s is here\r\n",usernext->name,GetPointerToFemaleTitles(usernext->status));
		}

		send(currentuser->handle,buf,strlen(buf),0);
	}

	usernext=FindNextUser(usernext);		/* find next user */
}

return(0);
}


/*
* looking at object or person
*/

objectnext=currentroom->roomobjects;

while(NULL != objectnext) {
	if(objectnext == NULL) break;

	if(regexp(objectnext->name,n) == TRUE) {
		send(currentuser->handle,objectnext->desc,strlen(objectnext->desc),0); /* if object matches */
		found=TRUE;
	}

	objectnext=objectnext->next;
}
	

/*
* if not not object or room, search for user
*/

usernext=FindFirstUser();		/* find first user */

while(usernext != NULL) {
	if((regexp(usernext->name,n) == TRUE) && (usernext->loggedin == TRUE) && (usernext->room == currentuser->room)) {
		sprintf(buf,"%s\r\n",usernext->desc);		/* show description */
		send(currentuser->handle,buf,strlen(buf),0);

		found=TRUE;

	/* if the user is a wizard tell them they have looked at them */

		sprintf(buf,"%s has looked at you\r\n",currentuser->name);
		if(usernext->status >= WIZARD) send(usernext->handle,buf,strlen(buf),0);
	}

	usernext=FindNextUser(usernext);		/* find next user */
}

/*
* if monster
*
*/

for(count=0;count<currentroom->monstercount;count++) {
	if(regexp(n,currentroom->roommonsters[count].name) == TRUE) {
		send(currentuser->handle,currentroom->roommonsters[count].desc,strlen(currentroom->roommonsters[count].desc),0);
		found=TRUE;
		return(0);
	}

}


/* can't find it, so output error message and exit */

if(found == FALSE) {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  
	return(-1);
}

return(0);
}



/* copy file */

int copyfile(char *source,char *destination) {
int sourcehandle;
int desthandle;
void *readbuf;
unsigned long result;
unsigned long count=0;
unsigned long countx;

sourcehandle=open(source,O_RDONLY);
if(sourcehandle == -1) return(-1);				/* can't open */

desthandle=creat(destination,0600);
if(desthandle == -1) return(-1);				/* can't open */

readbuf=calloc(1,BUF_SIZE);
if(readbuf == NULL) return(-1);	/* can't allocate */

count=-1;

while(count != 0) {
	count=read(sourcehandle,readbuf,BUF_SIZE);			/* copy data */

	if(count == -1) {
		free(readbuf);
		close(sourcehandle);
		close(desthandle);
		return(-1);
	}
	
	if(write(desthandle,readbuf,count) == -1) {
		free(readbuf);

		close(sourcehandle);
		close(desthandle);

		return(-1);
	}

}

free(readbuf);
close(sourcehandle);
close(desthandle);
return(0);
}  

/*
* update description for user or object
*/

int describe(user *currentuser,char *o,char *d) {
user *usernext;
roomobject *roomobject;
room *currentroom;
int count;
CONFIG config;

getconfigurationinformation(&config);

currentroom=currentuser->roomptr;

if(strcmp(o,"me") == 0) {                           /* if setting description for self */
	return(updateuser(currentuser,currentuser->name,"",0,0,d,0,currentuser->staminapoints,0,0,"","",0));
}

/*
* set description for other user
*
*/

usernext=FindFirstUser();		/* find first user */

while(usernext != NULL) {
	if((regexp(usernext->name,o) == TRUE) && (usernext->loggedin == TRUE) && (usernext->room == currentuser->room)) {
		if(currentuser->status < WIZARD) {  /* can't change other user's description unless you are wizard or higher */
			SetLastError(currentuser,NOT_YET);
			return(-1);
		}

		return(updateuser(currentuser,o,"",0,0,d,0,usernext->staminapoints,0,0,"","",0));
	}

	usernext=FindNextUser(usernext);		/* find next user */
}

/*
* set object description
*/
roomobject=currentroom->roomobjects;

while(roomobject != NULL) {
	if(regexp(roomobject->name,o) == TRUE) {		/* found user */
		/* if owner and OBJECT_RENAME_OWNER attribute not set, then display error */

		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(rooms[count].owner,currentuser->name) == 0) && (roomobject->attr & OBJECT_RENAME_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			/* if not owner and OBJECT_RENAME_PUBLIC attribute not set, then display error        */

			if((strcmp(rooms[count].owner,currentuser->name) != 0) && (roomobject->attr & OBJECT_RENAME_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}


		strcpy(roomobject->desc,d);               /* set object description	*/
		databaseupdated=TRUE;                     /* update database */
	}

	roomobject=roomobject->next;
}

	
if(strcmp(o,"here") == 0) {                           /* if setting description for room */
	if(currentuser->status < ARCHWIZARD) {
		if((strcmp(rooms[count].owner,currentuser->name) == 0) && (roomobject->attr & OBJECT_RENAME_OWNER) == 0) {
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
		}

	/* if not owner and OBJECT_RENAME_PUBLIC attribute not set, then display error        */

		if((strcmp(rooms[count].owner,currentuser->name) != 0) && (roomobject->attr & OBJECT_RENAME_PUBLIC) == 0) {
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
		}
	}

	strcpy(currentroom->desc,d);
	databaseupdated=TRUE;

	return(0);
}

/*
* set room description
*/

for(count=0;count<config.roomcount;count++) {
	if(regexp(rooms[count].name,o) == TRUE) {                    /* found object */
		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(rooms[count].owner,currentuser->name) == 0) && (roomobject->attr & OBJECT_RENAME_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			/* if not owner and OBJECT_RENAME_PUBLIC attribute not set, then display error        */

			if((strcmp(rooms[count].owner,currentuser->name) != 0) && (roomobject->attr & OBJECT_RENAME_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}


		strcpy(rooms[count].desc,d);                              /* set object description */
		databaseupdated=TRUE;                               /* update database */
		return(0);
	}

}

SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}

void setdatabaseupdatedflag(void) {
	databaseupdated=TRUE;
}

void cleardatabaseupdatedflag(void) {
	databaseupdated=FALSE;
}

char *GetDirectionName(int direction) {
return(&directions[direction]);
}

int GetRoomFlags(int roomnumber) {
return(rooms[roomnumber].attr);
}

char *GetRoomName(int roomnumber) {
return(&rooms[roomnumber].name);
}

char *GetRoomPointer(int roomnumber) {
return(&rooms[roomnumber]);
}

int GetRoomMonsterEvil(int roomnumber,int roommonster) {
return(rooms[roomnumber].roommonsters[roommonster].evil);
}

char *GetRoomMonsterName(int roomnumber,int roommonster) {
return(&rooms[roomnumber].roommonsters[roommonster].name);
}

int GetNumberOfMonstersInRoom(int roomnumber) {
return(rooms[roomnumber].monstercount);
}

