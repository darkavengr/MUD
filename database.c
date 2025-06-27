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
#include "monster.h"
#include "database.h"

char *DatabaseConfigurationFile="config/database.mud";                                          /* configuration files */
int DatabaseUpdated;
char *directions[]={ "north","south","east","west","northeast","northwest","southeast","southwest","up","down" };
char *ObjectsConfigurationFile="config/reset.mud";
room *rooms=NULL;
unsigned int NumberOfRooms=0;
unsigned int NextObjectNumber=0;

/* update database file */

int UpdateDatabase(void) {
FILE *handle;
char *NewDatabaseName[BUF_SIZE];
time_t rawtime;
struct tm *timeinfo;
roomobject *roomobjects;
CONFIG config;
char *CurrentDirectory[BUF_SIZE];
char *BackupFilename[BUF_SIZE];
int RoomCount;

GetConfigurationInformation(&config);

time(&rawtime);	/* get time for backup filename */
timeinfo=localtime(&rawtime);

/* if configured to backup, copy database to backup file */

if(config.databasebackup == TRUE) {
	strftime(NewDatabaseName,BUF_SIZE,"config/database-%H-%M-%S-%d-%e.sav",timeinfo);	/* get backup name */

	sprintf(BackupFilename,"%s/%s",CurrentDirectory,NewDatabaseName);		/* get path of backup */

	CopyFile(DatabaseConfigurationFile,BackupFilename);	/* backup database */
}

handle=fopen(DatabaseConfigurationFile,"w");
if(handle == NULL) return(-1);	/* can't open file */

/*
* for each room output to file
*/
for(RoomCount=0;RoomCount<GetNumberOfRooms();RoomCount++) {
	if(rooms[RoomCount].id > 0) {
	fprintf(handle,"begin_room:%d\n",rooms[RoomCount].id);
	fprintf(handle,"name:%s\n",rooms[RoomCount].name);
	fprintf(handle,"owner:%s\n",rooms[RoomCount].owner);
	fprintf(handle,"attr:%d\n",rooms[RoomCount].attr);
	fprintf(handle,"description:%s\n",rooms[RoomCount].desc);
	fprintf(handle,"north:%d\n",rooms[RoomCount].exits[NORTH]);
	fprintf(handle,"south:%d\n",rooms[RoomCount].exits[SOUTH]);
	fprintf(handle,"east:%d\n",rooms[RoomCount].exits[EAST]);
	fprintf(handle,"west:%d\n",rooms[RoomCount].exits[WEST]);
	fprintf(handle,"northeast:%d\n",rooms[RoomCount].exits[NORTHEAST]);
	fprintf(handle,"southeast:%d\n",rooms[RoomCount].exits[SOUTHEAST]);
	fprintf(handle,"northwest:%d\n",rooms[RoomCount].exits[NORTHWEST]);
	fprintf(handle,"northeast:%d\n",rooms[RoomCount].exits[NORTHEAST]);
	fprintf(handle,"up:%d\n",rooms[RoomCount].exits[UP]);
	fprintf(handle,"down:%d\n",rooms[RoomCount].exits[DOWN]);

	/*
	* output objects
	*/

	roomobjects=rooms[RoomCount].roomobjects;

	while(roomobjects != NULL) {
		if(roomobjects->attr & OBJECT_TEMPORARY) {
			roomobjects=roomobjects->next;    
			continue;
		}

		if(*roomobjects->name) fprintf(handle,"object:%s:%d:%d:%d:%d:%s:%s:%d:%s:%s\n",
			roomobjects->id,\
			roomobjects->name,\
			roomobjects->staminapoints,\
			roomobjects->magicpoints,\
			roomobjects->attackpoints,\
			roomobjects->generateprob,\
			roomobjects->desc,\
			roomobjects->owner,\
			roomobjects->attr,\
			roomobjects->verb,\
			roomobjects->verbmessage);
	
			roomobjects=roomobjects->next;
		}
	}
}

fclose(handle);
return(0);
}

int LoadDatabase(void) {
FILE *handle;
int LineCount=0;
roomobject *roomobject;
char *LineBuffer[BUF_SIZE];
char *LineTokens[BUF_SIZE][BUF_SIZE];
int DirectionCount;
int ErrorCount=0;
CONFIG config;

NextObjectNumber=1 << ((sizeof(unsigned int)*8)-1);		/* first object number */

GetConfigurationInformation(&config);

/* get path of database */

handle=fopen(DatabaseConfigurationFile,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",DatabaseConfigurationFile);
	exit(NOCONFIGFILE);
}

NumberOfRooms=0;

/* room list is an array, not a linked list because it is randomly accessed */

rooms=calloc(1,sizeof(room));	/* allocate unused room, room numbers start from 1 */
if(rooms == NULL) return(-1);

while(!feof(handle)) {
	fgets(LineBuffer,BUF_SIZE,handle);	/*' get and parse line */

	if((char) *LineBuffer == '#')  continue;	/* skip comments */
	if((char) *LineBuffer == '\n')  continue;	/* skip newline */
	
	RemoveNewLine(LineBuffer);	/* remove newline character */

	TokenizeLine(LineBuffer,LineTokens,":\n");	/* tokenize line */

	LineCount++;

	if(strcmp(LineTokens[0],"begin_room") == 0) {	/* room name */
		NumberOfRooms++;

		rooms=realloc(rooms,sizeof(room)*(NumberOfRooms+1));
		if(rooms == NULL) return(-1);
	
		rooms[NumberOfRooms].id=atoi(LineTokens[1]);	/* get room number (ID) */
	}
	else if(strcmp(LineTokens[0],"name") == 0) {	/* room name */
		strcpy(rooms[NumberOfRooms].name,LineTokens[1]);
	}
	else if(strcmp(LineTokens[0],"owner") == 0) {	/* owner */
		strcpy(rooms[NumberOfRooms].owner,LineTokens[1]);
	}
	else if(strcmp(LineTokens[0],"attr") == 0) {	/* attributes */
		rooms[NumberOfRooms].attr=atoi(LineTokens[1]);
	}
	else if(strcmp(LineTokens[0],"description") == 0) {	/* description */
		strcpy(rooms[NumberOfRooms].desc,LineTokens[1]);
	}
	else if(strcmp(LineTokens[0],"name") == 0) {
		strcpy(rooms[NumberOfRooms].name,LineTokens[1]);
	}
	else if(strcmp(LineTokens[0],"object") == 0) {	/* room object */

		if(rooms[NumberOfRooms].roomobjects == NULL) {	/* first object */
			rooms[NumberOfRooms].roomobjects=calloc(1,sizeof(roomobject));	/* add link to end */

			if(rooms[NumberOfRooms].roomobjects == NULL) return(-1);	/* can't allocate */
	
			roomobject=rooms[NumberOfRooms].roomobjects;
			rooms[NumberOfRooms].roomobjects_last=rooms[NumberOfRooms].roomobjects;
		}
		else
		{
			rooms[NumberOfRooms].roomobjects_last->next=calloc(1,sizeof(roomobject));	/* add link to end */
			if(rooms[NumberOfRooms].roomobjects_last->next == NULL) return(-1);	/* can't allocate */
		
			rooms[NumberOfRooms].roomobjects_last=rooms[NumberOfRooms].roomobjects_last->next;
			roomobject=rooms[NumberOfRooms].roomobjects_last;
		}

		roomobject->id=GetNextObjectNumber();

		strcpy(roomobject->name,LineTokens[OBJECT_NAME]);

		roomobject->attr=atoi(LineTokens[OBJECT_ATTR]);

		strcpy(roomobject->desc,LineTokens[OBJECT_DESCRIPTION]);	

		roomobject->attackpoints=atoi(LineTokens[OBJECT_ATTACKPOINTS]);	
		roomobject->staminapoints=atoi(LineTokens[OBJECT_GENERATEPROB]);
		roomobject->magicpoints=atoi(LineTokens[OBJECT_MAGICPOINTS]);  

		strcpy(roomobject->verb,LineTokens[OBJECT_VERB]);
		strcpy(roomobject->verbmessage,LineTokens[OBJECT_VERB_MESSAGE]);
		roomobject->next=NULL;
	}
	else if(strcmp(LineTokens[0],"end") == 0) {
		;;
	}

	/* handle room exits */

	for(DirectionCount=0;DirectionCount != NUMBER_OF_DIRECTIONS;DirectionCount++) {
		if(strcmp(LineTokens[0],directions[DirectionCount]) == 0) {	/* room exits */
			rooms[NumberOfRooms].exits[DirectionCount]=atoi(LineTokens[1]);
			break;
		}
	}

	if(DirectionCount < 12) continue;	/* was a north, south, east, west,... statement */

	if((char) *LineTokens[0] == '#') continue;		/* comment */

	/* unknown configuration option */

	printf("\nmud: %d: Unknown configuration option %s in %s\n",LineCount,LineTokens[0],DatabaseConfigurationFile);
	ErrorCount++;
	continue;
}

fclose(handle);

return(ErrorCount);
}

int SetExit(user *currentuser,int whichroom,int direction,int exit) {
if(currentuser->status < ARCHWIZARD) {	/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(direction > 11) {
	SetLastError(currentuser,INVALID_EXIT);
	return(-1);
}

if(direction > GetNumberOfRooms()) {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  
	return(-1);
}

rooms[whichroom].exits[direction]=exit;
return(0);
}

/* create room */

int CreateRoom(user *currentuser,char *roomdirection) {
int LastRoom;
int RoomDirectionCount;
char *CreateMessage[BUF_SIZE];


if(currentuser->status < WIZARD) {	/* must be wizard or higher level to create room */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/* deny access if not owner and not allowed to create exit */

if(currentuser->status < ARCHWIZARD) {
	if((strcmp(rooms[currentuser->room].owner,currentuser->name) !=0) && (rooms[currentuser->room].attr  & ROOM_EXIT_PUBLIC) == 0 && currentuser->status < ARCHWIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	/* deny permission if not owner not allowed to create objects in room */

	if((strcmp(rooms[currentuser->room].owner,currentuser->name) == 0) && (rooms[currentuser->room].attr  & ROOM_EXIT_OWNER) == 0 && currentuser->status < ARCHWIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}
}


/* if the room is in argument, create it */

if(*roomdirection) {		/* room spcified */
	for(RoomDirectionCount=0;RoomDirectionCount != 11;RoomDirectionCount++) {
		if(strcmp(roomdirection,directions[RoomDirectionCount]) == 0) break;	/* found */
	}
}
else
{
	for(RoomDirectionCount=0;RoomDirectionCount != 11;RoomDirectionCount++) {
		if(rooms[currentuser->room].exits[RoomDirectionCount] == 0) break;	/* found */
	}
}

if(RoomDirectionCount > 11) { 
	SetLastError(currentuser,BAD_DIRECTION);  
	return(-1);
}

LastRoom=GetNumberOfRooms();
LastRoom++;

rooms=realloc(rooms,(sizeof(room)*LastRoom)+1);	/* resize database */
if(rooms == NULL) {
	SetLastError(currentuser,NO_MEM);  
	return(-1);
}

memset(&rooms[LastRoom],0,sizeof(room));	/* clear room */
strcpy(rooms[LastRoom].name,"Empty room\r\r\n");	/* create room info */
strcpy(rooms[LastRoom].owner,currentuser->name);
strcpy(rooms[LastRoom].desc,"Empty room, you can describe it using desc here <description>\r\n");

rooms[LastRoom].attr=ROOM_CREATE_OWNER | ROOM_EXIT_OWNER | ROOM_RENAME_OWNER;
rooms[LastRoom].id=LastRoom;
rooms[LastRoom].roomobjects=NULL;

currentuser->roomptr->exits[RoomDirectionCount]=GetNextObjectNumber();
	
DatabaseUpdated=TRUE;                  /* update database flag */

SetLastError(currentuser,NO_ERROR);
return(0);
}

/* set an object, room or user's attributes */

int SetObjectAttributes(user *currentuser,unsigned int ObjectID,int attributes) {
roomobject *ObjectPtr;
int RoomCount;
char *ErrorMessage[BUF_SIZE];


if(currentuser->status < WIZARD) {            /* need to be wizard or higher level to change attributes */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

ObjectPtr=currentuser->roomptr->roomobjects;	/* point to objects */

while(ObjectPtr != NULL) {

	if(ObjectPtr->id == ObjectID) {	/* found object */
		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(ObjectPtr->owner,currentuser->name) == 0) && (ObjectPtr->attr & OBJECT_MOVEABLE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
		}

		if((strcmp(ObjectPtr->owner,currentuser->name) == 0) && (ObjectPtr->attr & OBJECT_MOVEABLE_OWNER) == 0) {
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
		}
	}

	ObjectPtr->attr=attributes;

	DatabaseUpdated=TRUE;

	SetLastError(currentuser,NO_ERROR);
	return(0);
	}


	ObjectPtr=ObjectPtr->next;
}


/* Setting a room's attribute */

for(RoomCount=0;RoomCount<GetNumberOfRooms();RoomCount++) {

	if(rooms[RoomCount].id == ObjectID) {	/* found room */
	rooms[RoomCount].attr=attributes;

	SetLastError(currentuser,NO_ERROR);
	return(0);
	}
}

SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}


/* set owner of room or object */

int SetOwner(user *currentuser,unsigned int ObjectID,char *owner) {
room *roomnext;
roomobject *ObjectPtr;
int RoomCount;


if(currentuser->status < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}
	
if(currentuser->status <ARCHWIZARD) {

	if(strcmp(owner,currentuser->name) == 0) {                    /* unless archwizard, or higher, must be object's owner to modify */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
	} 
}

/* find object */

ObjectPtr=currentuser->roomptr->roomobjects;
while(ObjectPtr != NULL) {

	if(ObjectPtr->id == ObjectID) { 		/* if object found */
		strcpy(ObjectPtr->owner,owner);
		DatabaseUpdated=TRUE;

		return(0);
	}

	ObjectPtr=ObjectPtr->next;
}

/*  find room */

for(RoomCount=0;RoomCount<GetNumberOfRooms();RoomCount++) {
	if(rooms[RoomCount].id == ObjectID) {	/* if object found */
		if(currentuser->status < ARCHWIZARD && strcmp(rooms[RoomCount].owner,currentuser->name) != 0) {	/* permission denied */
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
		}

		strcpy(rooms[RoomCount].owner,owner);   

		DatabaseUpdated=TRUE;       		/* update database */
		return(0);
	}

}
	
SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(0);
}

/* copy object */

int CopyObject(user *currentuser,unsigned int ObjectID,int DestinationRoom) {
roomobject *ObjectPtr;
user *usernext;
int found=FALSE;
int count;
int CopyDestination;


if(currentuser->status < WIZARD) {      /* not wizard */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(GetNumberOfRooms() > currentuser->roomptr->id) {		/* can't find room */
	SetLastError(currentuser,OBJECT_NOT_FOUND);  
	return(-1);
}

/* move object */
	
ObjectPtr=currentuser->roomptr->roomobjects;

while(ObjectPtr != NULL) {
	if(ObjectPtr->id == ObjectID) {	/* if object matches */

	if(currentuser->status < ARCHWIZARD) {
		if((strcmp(currentuser->roomptr->owner,currentuser->name) == 0) && (currentuser->roomptr->attr & OBJECT_MOVEABLE_PUBLIC) == 0) {
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
		}

		if((strcmp(currentuser->roomptr->owner,currentuser->name) == 0) && (currentuser->roomptr->attr & OBJECT_MOVEABLE_OWNER) == 0) {
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
		}
	}

	/* copy object */
	if(rooms[DestinationRoom].roomobjects == NULL) {
		rooms[DestinationRoom].roomobjects=calloc(1,sizeof( roomobject));	/* allocate objects */ 
		if(rooms[DestinationRoom].roomobjects == NULL) {	/* can't allocate */
			SetLastError(currentuser,NO_MEM);  
			return(-1);
		}

		rooms[DestinationRoom].roomobjects_last=rooms[DestinationRoom].roomobjects;
	}
	else
	{
		rooms[DestinationRoom].roomobjects_last->next=calloc(1,sizeof( roomobject));	/* allocate objects */ 
		if(rooms[DestinationRoom].roomobjects == NULL) {	/* can't allocate */
	SetLastError(currentuser,NO_MEM);  
	return(-1);
		}

		rooms[DestinationRoom].roomobjects_last=rooms[DestinationRoom].roomobjects_last->next;
	}

	memcpy(rooms[DestinationRoom].roomobjects_last,ObjectPtr,sizeof(roomobject));	/* copy object */
	
	rooms[DestinationRoom].roomobjects_last->id=GetNextObjectNumber();	/* generate new ID number for copied object */
	found=TRUE;
	}

	ObjectPtr=ObjectPtr->next;
}

SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}

/* generate objects */

int GenerateObjects(void) {
FILE *handle;
char *tokens[BUF_SIZE][BUF_SIZE];
int NumberOfObjectsToGenerate=0;
int RandomObjectNumber;
int TotalObjectCount=0;
char *ObjectsList=NULL;
int RoomObjectCount;
int RoomCount=0;
CONFIG config;

GetConfigurationInformation(&config);

handle=fopen(ObjectsConfigurationFile,"rb");
if(handle == NULL) return(-1);	/* can't open configution file */

/*
* load objects into array */

while(!feof(handle)) {
	if(ObjectsList == NULL) {
		ObjectsList=malloc(BUF_SIZE);
		if(ObjectsList == NULL) return(-1);
	}
	else
	{
		TotalObjectCount++;

		ObjectsList=realloc(ObjectsList,BUF_SIZE*(TotalObjectCount+1));
		if(ObjectsList == NULL) return(-1);
	}

	fgets(&ObjectsList[TotalObjectCount*BUF_SIZE],BUF_SIZE,handle);                                      /* get line into array */
}

fclose(handle);

srand(time(NULL));

/* Generate room objects */

for(RoomCount=1;RoomCount < GetNumberOfRooms(); RoomCount++) {

//	if((rooms[RoomCount].attr & ROOM_HAVEN) == 0) {          /* not haven */
		NumberOfObjectsToGenerate=rand() % (config.roomobjectnumber + 1) - 0;

		while(NumberOfObjectsToGenerate > 0) {

			RandomObjectNumber=rand() % TotalObjectCount;

			TokenizeLine(&ObjectsList[RandomObjectNumber*BUF_SIZE],tokens,":");	/* tokenize line */
		
			if(IsObjectInRoom(tokens[OBJECT_NAME],RoomCount) == FALSE) { 	/* if object is not in room */

				if(rooms[RoomCount].roomobjects == NULL) {	/* no objects in room */
					rooms[RoomCount].roomobjects=calloc(1,sizeof(roomobject));	/* add link to end */
					if(rooms[RoomCount].roomobjects == NULL) continue;

					rooms[RoomCount].roomobjects_last=rooms[RoomCount].roomobjects;
				}
	        		else
	        		{
					rooms[RoomCount].roomobjects_last->next=calloc(1,sizeof(roomobject));	/* add link to end */
					if(rooms[RoomCount].roomobjects_last->next == NULL) continue;

					rooms[RoomCount].roomobjects_last=rooms[RoomCount].roomobjects_last->next;
				}

//				printf("Generating %s in room %d\n",tokens[OBJECT_NAME],RoomCount);

				/* add object to room */

				rooms[RoomCount].roomobjects_last->id=GetNextObjectNumber();	/* generate new ID number for copied object */
				strcpy(rooms[RoomCount].roomobjects_last->name,tokens[OBJECT_NAME]);
				strcpy(rooms[RoomCount].roomobjects_last->owner,"nobody");
				strcpy(rooms[RoomCount].roomobjects_last->desc,tokens[OBJECT_DESCRIPTION]);
				rooms[RoomCount].roomobjects_last->attackpoints=atoi(tokens[OBJECT_ATTACKPOINTS]);
				rooms[RoomCount].roomobjects_last->generateprob=atoi(tokens[OBJECT_GENERATEPROB]);
				rooms[RoomCount].roomobjects_last->staminapoints=atoi(tokens[OBJECT_STAMINAPOINTS]);
				rooms[RoomCount].roomobjects_last->magicpoints=atoi(tokens[OBJECT_MAGICPOINTS]);
				rooms[RoomCount].roomobjects_last->attr=OBJECT_TEMPORARY;
				strcpy(rooms[RoomCount].roomobjects_last->verb,tokens[OBJECT_VERB]);
				strcpy(rooms[RoomCount].roomobjects_last->verbmessage,tokens[OBJECT_VERB_MESSAGE]);
			}  

			NumberOfObjectsToGenerate--;
		}
//	}

}

free(ObjectsList);

return(0);
}

/* create object */

int CreateObject(user *currentuser,char *ObjectName) {
roomobject *ObjectPtr;
roomobject *ObjPtrLast;
char *buf[BUF_SIZE];


if(currentuser->status < WIZARD) {         /* can't create object unless wizard level or above */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/* deny permission if not archwizard and the owner doesn't allow others to create objects in room */

if(currentuser->status < ARCHWIZARD) {
	if((strcmp(currentuser->roomptr->owner,currentuser->name) !=0) && (currentuser->roomptr->attr  & ROOM_CREATE_PUBLIC) == 0 && currentuser->status < ARCHWIZARD) {
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
	}

/* deny permission if not archwizard and owner not allowed to create objects in room */

	if((strcmp(currentuser->roomptr->owner,currentuser->name) == 0) && (currentuser->roomptr->attr  & ROOM_CREATE_OWNER) == 0 && currentuser->status < ARCHWIZARD) {
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
	}
}

/* check if object already exists and display error message if it does */

ObjectPtr=currentuser->roomptr->roomobjects;

while(ObjectPtr != NULL) {
	
	if(strcmp(ObjectName,ObjectPtr->name) == 0) {
	SetLastError(currentuser,OBJECT_EXISTS);  
	return(-1);
	}

	ObjPtrLast=ObjectPtr;
	ObjectPtr=ObjectPtr->next;
}
	
if(currentuser->roomptr->roomobjects == NULL) {	/* no objects */
	currentuser->roomptr->roomobjects=calloc(1,sizeof(roomobject));	/* add link to end */
	if(currentuser->roomptr->roomobjects == NULL) {	/* can't allocate */
	SetLastError(currentuser,NO_MEM);  
	return(-1);
	}

	ObjectPtr=currentuser->roomptr->roomobjects;
}
else
{
	ObjectPtr=currentuser->roomptr->roomobjects_last;
}


ObjectPtr->next=calloc(1,sizeof(roomobject));
if(ObjectPtr == NULL) {	/* can't allocate */
	SetLastError(currentuser,NO_MEM);  
	return(-1);
}

ObjectPtr=ObjectPtr->next;

/*
* add object details at end
*/

strcpy(ObjectPtr->name,ObjectName);
ObjectPtr->attr=OBJECT_DELETE_OWNER+OBJECT_MOVEABLE_OWNER+OBJECT_RENAME_OWNER;
ObjectPtr->magicpoints=0;
strcpy(ObjectPtr->owner,currentuser->name);
strcpy(ObjectPtr->desc,"No description");

ObjectPtr->id=GetNextObjectNumber();	/* generate new ID number for created object */

ObjectPtr->next=NULL;

DatabaseUpdated=TRUE;                                     /* mark database as updated */
return(0);
}

/* delete object */

int DeleteObject(user *currentuser,unsigned int ObjectID) {
roomobject *ObjectPtr;
roomobject *ObjPtrLast;
roomobject *OldPtr;

if(currentuser->status < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

ObjectPtr=currentuser->roomptr->roomobjects;

ObjPtrLast=ObjectPtr;

while(ObjectPtr != NULL) {
	if(ObjectPtr->id == ObjectID) {	/* found */
		/* can't delete if not owner and OBJECT_DELETE_PUBLIC attribute not set */

		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(ObjectPtr->owner,currentuser->name) != 0) && (ObjectPtr->attr & OBJECT_DELETE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			/* object owner and OBJECT_MOVEABLE_OWNER attribute not set, display error message */

			if((strcmp(ObjectPtr->owner,currentuser->name) == 0) && (ObjectPtr->attr & OBJECT_DELETE_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		/* remove object from room */

		if(ObjectPtr == currentuser->roomptr->roomobjects) {	/* first object */
			OldPtr=currentuser->roomptr->roomobjects;
	
			currentuser->roomptr->roomobjects=currentuser->roomptr->roomobjects->next;

			free(OldPtr);
		}
		else if(ObjectPtr->next == NULL) {	/* last object */
			ObjPtrLast->next=NULL;	/* new end of list */
			currentuser->roomptr->roomobjects_last=ObjPtrLast;	/* save new end of list */

			free(ObjectPtr);
		}
		else {
			ObjPtrLast->next=ObjectPtr->next;	/* skip over over object */
			free(ObjectPtr);
		}

		DatabaseUpdated=TRUE;                                       /* database update flag */
		return(0);
	}

	ObjPtrLast=ObjectPtr;
	ObjectPtr=ObjectPtr->next;
}

SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}

int RenameObject(user *currentuser,unsigned int ObjectID,char *newname) {
user *usernext;
roomobject *ObjectPtr;

 
if(currentuser->status < WIZARD) {	/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/* renaming object */
ObjectPtr=currentuser->roomptr->roomobjects;

	while(ObjectPtr->next != NULL) {

	if(ObjectPtr->id == ObjectID) {	/* found object */
		/* check permissions */

		if(currentuser->status < ARCHWIZARD) {

		/* is owner but can't rename */

	if((strcmp(ObjectPtr->owner,currentuser->next) == 0) && ((ObjectPtr->attr & OBJECT_RENAME_OWNER) == 0)) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

		/* not owner and can't rename */

		if((strcmp(ObjectPtr->owner,currentuser->next) != 0) && ((ObjectPtr->attr & OBJECT_RENAME_PUBLIC) == 0)) {
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
		}
	}

	strcpy(ObjectPtr->name,newname);	/* rename object */

	DatabaseUpdated=TRUE;
	return(0);
	}

	ObjectPtr=ObjectPtr->next;
}

SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}

int CopyFile(char *source,char *destination) {
int sourcehandle;
int desthandle;
void *readbuf;
unsigned long result;
unsigned long count=0;
unsigned long countx;

sourcehandle=open(source,O_RDONLY);
if(sourcehandle == -1) return(-1);	/* can't open */

desthandle=creat(destination,0600);
if(desthandle == -1) return(-1);	/* can't open */

readbuf=calloc(1,BUF_SIZE);
if(readbuf == NULL) return(-1);	/* can't allocate */

count=-1;

while(count != 0) {
	count=read(sourcehandle,readbuf,BUF_SIZE);		/* copy data */

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

int SetObjectDescription(user *currentuser,unsigned int ObjectID,char *description) {
roomobject *roomobject;

roomobject=currentuser->roomptr->roomobjects;

while(roomobject != NULL) {
	if(roomobject->id == ObjectID) {	/* found object */
		/* if owner and OBJECT_RENAME_OWNER attribute not set, then display an error message */

		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(roomobject->owner,currentuser->name) == 0) && (roomobject->attr & OBJECT_RENAME_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			/* if not owner and OBJECT_RENAME_PUBLIC attribute not set, then display an error message */

			if((strcmp(roomobject->owner,currentuser->name) != 0) && (roomobject->attr & OBJECT_RENAME_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}


		strcpy(roomobject->desc,description);               /* set object description	*/
		DatabaseUpdated=TRUE;                     /* update database */

		SetLastError(currentuser,NO_ERROR);
		return(0);
	}

	roomobject=roomobject->next;
}


SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}

void SetDatabaseUpdateFlag(void) {
DatabaseUpdated=TRUE;
}

void ClearDatabaseUpdateFlag(void) {
DatabaseUpdated=FALSE;
}

char *GetDirectionName(int direction) {
return(&directions[direction]);
}

char *GetRoomName(int RoomNumber) {
return(&rooms[RoomNumber].name);
}

room *GetRoomPointer(int RoomNumber) {
return(&rooms[RoomNumber]);
}

int GetRoomExit(int RoomNumber,int direction) {
return(rooms[RoomNumber].exits[direction]);
}

int GetRoomAttributes(int RoomNumber) {
return(rooms[RoomNumber].attr);
}

void SetFirstMonsterInRoom(int RoomNumber,monster *Monster) {
rooms[RoomNumber].roommonsters=Monster;
}

int IsObjectInRoom(char *name,int RoomNumber) {
roomobject *roomptr;

roomptr=rooms[RoomNumber].roomobjects;

/* search through list of objects */

while(roomptr != NULL) {
	if(strcmp(name,roomptr->name) == 0) return(TRUE);	/* found object */

	roomptr=roomptr->next;
}

return(FALSE);
}

int GetNumberOfRooms(void) {
return(NumberOfRooms);
}

unsigned int GetNextObjectNumber(void) {
return(NextObjectNumber++);
}

int SetRoomDescription(user *currentuser,int RoomNumber,char *description) {
if(RoomNumber > GetNumberOfRooms()) {	/* sanity check */
	SetLastError(currentuser,OBJECT_NOT_FOUND);
	return(-1);
}

strcpy(rooms[RoomNumber].desc,description);	/* set description */

DatabaseUpdated=TRUE;

SetLastError(currentuser,NO_ERROR);
return(0);
}

int AddMonsterToRoom(monster *sourcemonster,int RoomNumber) {
if(rooms[RoomNumber].roommonsters == NULL) {
	rooms[RoomNumber].roommonsters=calloc(1,sizeof(monster));	/* allocate objects */
	if(rooms[RoomNumber].roommonsters == NULL) return(-1);	/* can't allocate */

	rooms[RoomNumber].roommonsters_last=rooms[RoomNumber].roommonsters;
}
else
{
	rooms[RoomNumber].roommonsters_last->next=calloc(1,sizeof(monster));	/* allocate objects */
	if(rooms[RoomNumber].roommonsters_last->next == NULL) return(-1);	/* can't allocate */
	
	rooms[RoomNumber].roommonsters_last=rooms[RoomNumber].roommonsters_last->next;
}

memcpy(rooms[RoomNumber].roommonsters_last,sourcemonster,sizeof(monster));	/* copy monster data */

rooms[RoomNumber].roommonsters_last->id=GetNextObjectNumber();	/* assign an ID number to the monster */
rooms[RoomNumber].roommonsters_last->next=NULL;

printf("Created monster %s in room %d\n",rooms[RoomNumber].roommonsters_last->name,RoomNumber);
return(0);
}

monster *FindFirstMonsterInRoom(int RoomNumber) {
return(rooms[RoomNumber].roommonsters);
}

monster *FindNextMonsterInRoom(monster *previous) {
return(previous->next);
}

void SetNumberOfMonstersInRoom(int RoomNumber,int NumberOfMonsters) {
rooms[RoomNumber].monstercount=NumberOfMonsters;
}

int GetNumberOfMonstersInRoom(int RoomNumber) {
return(rooms[RoomNumber].monstercount);
}

int DoObjectVerbAction(user *currentuser,char *verb,char *ObjectName) {
roomobject *ObjectPtr;
char *VerbMessage[BUF_SIZE];

printf("object verb action\n");

printf("currentuser->roomptr=%lX\n",currentuser->roomptr);

ObjectPtr=currentuser->roomptr->roomobjects;

printf("ObjectPtr=%lX\n",ObjectPtr);

while(ObjectPtr != NULL) {
	printf("ObjectPtr->name=%s ** %s\n",ObjectPtr->name,ObjectName);

	if(strcmp(ObjectPtr->name,ObjectName) == 0) {		/* found object */
		printf("Found object\n");

		printf("verb=%s ** %s\n",verb,ObjectPtr->verb);

		ToUppercase(ObjectPtr->verb);

		if(strcmp(verb,ObjectPtr->verb) == 0) {	/* found verb */	
			printf("Found verb\n");

			send(currentuser->handle,ObjectPtr->verbmessage,strlen(ObjectPtr->verbmessage),0);

			DeleteObject(currentuser,ObjectPtr->id);
			SetLastError(currentuser,NO_ERROR);
			return(0);
		}

		SetLastError(currentuser,VERB_NOT_FOUND);
		return(-1);
	}

	ObjectPtr=ObjectPtr->next;
}

SetLastError(currentuser,OBJECT_NOT_FOUND);
return(-1);
}
