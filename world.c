/* database function */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sqlite3.h>
#include <stdbool.h>

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

#include <time.h>

#include "bool.h"
#include "directions.h"
#include "errors.h"
#include "user.h"
#include "room.h"
#include "config.h"
#include "monster.h"
#include "world.h"
#include "getconfig.h"
#include "string.h"

char *directions[]={ "north","south","east","west","northeast","northwest","southeast","southwest","up","down" };
room *rooms=NULL;
unsigned int NumberOfRooms=0;
unsigned int NextObjectNumber=0;

int UpdateRoomInDatabase(user *user,int room) {
FILE *handle;
char *NewDatabaseName[BUF_SIZE];
time_t rawtime;
struct tm *timeinfo;
CONFIG config;
char *CurrentDirectory[BUF_SIZE];
char *BackupFilename[BUF_SIZE];
int RoomCount;
sqlite3_stmt *SQLStatementHandle;

GetConfigurationInformation(&config);

time(&rawtime);	/* get time for backup filename */
timeinfo=localtime(&rawtime);

/* if configured to backup, copy database to backup file */

if(config.BackupDatabase == TRUE) {
	strftime(NewDatabaseName,BUF_SIZE,"config/database-%H-%M-%S-%d-%e.sav",timeinfo);	/* get backup name */

	sprintf(BackupFilename,"%s/%s",CurrentDirectory,NewDatabaseName);		/* get path of backup */

	CopyFile(GetDatabaseFilename(),BackupFilename);	/* backup database */
}

/* prepare SQL statement */
if(sqlite3_prepare_v2(GetDatabaseHandle(),"UPDATE WORLD SET name=?,owner=?,attributes=?,description=?,north=?,south=?,east=?,west=?,northeast=?,northwest=?,southeast=?,southwest=?,up=?,down=? WHERE id=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	
	SetLastError(user,IO_ERROR);
	return(-1);
}

/* bind parameters */
sqlite3_bind_text(SQLStatementHandle,1,rooms[room].name,strlen(rooms[room].name),NULL);
sqlite3_bind_text(SQLStatementHandle,2,rooms[room].owner,strlen(rooms[room].owner),NULL);
sqlite3_bind_int(SQLStatementHandle,3,rooms[room].attributes);
sqlite3_bind_text(SQLStatementHandle,4,rooms[room].description,strlen(rooms[room].description),NULL);
sqlite3_bind_int(SQLStatementHandle,5,rooms[room].exits[NORTH]);
sqlite3_bind_int(SQLStatementHandle,6,rooms[room].exits[SOUTH]);
sqlite3_bind_int(SQLStatementHandle,7,rooms[room].exits[EAST]);
sqlite3_bind_int(SQLStatementHandle,8,rooms[room].exits[WEST]);
sqlite3_bind_int(SQLStatementHandle,9,rooms[room].exits[NORTHEAST]);
sqlite3_bind_int(SQLStatementHandle,10,rooms[room].exits[NORTHWEST]);
sqlite3_bind_int(SQLStatementHandle,11,rooms[room].exits[SOUTHEAST]);
sqlite3_bind_int(SQLStatementHandle,12,rooms[room].exits[SOUTHWEST]);
sqlite3_bind_int(SQLStatementHandle,13,rooms[room].exits[UP]);
sqlite3_bind_int(SQLStatementHandle,14,rooms[room].exits[DOWN]);
sqlite3_bind_int(SQLStatementHandle,15,room);

if(sqlite3_step(SQLStatementHandle) == SQLITE_DONE) {		/* update OK */
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(0);
}

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
return(0);
}

int UpdateObjectInDatabase(user *currentuser,roomobject *roomobject) {
sqlite3_stmt *SQLStatementHandle;

/* don't update temporary objects or objects that have not been updated */

if(roomobject->attributes & OBJECT_TEMPORARY) return(0);

/* prepare statement SQL */
if(sqlite3_prepare_v2(GetDatabaseHandle(),"UPDATE WORLDOBJECTS SET name=?,staminapoints=?,magicpoints=?,description=?,owner=?,attributes=?,verb=?,verbmessage=? WHERE id=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	printf("mud: %s\n",sqlite3_errmsg(GetDatabaseHandle()));

	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

/* bind parameters */
	
sqlite3_bind_text(SQLStatementHandle,1,roomobject->name,strlen(roomobject->name),NULL);
sqlite3_bind_int(SQLStatementHandle,2,roomobject->staminapoints);
sqlite3_bind_int(SQLStatementHandle,3,roomobject->magicpoints);
sqlite3_bind_text(SQLStatementHandle,4,roomobject->description,strlen(roomobject->description),NULL);
sqlite3_bind_text(SQLStatementHandle,5,roomobject->owner,strlen(roomobject->owner),NULL);
sqlite3_bind_int(SQLStatementHandle,6,roomobject->attributes);
sqlite3_bind_text(SQLStatementHandle,7,roomobject->verb,strlen(roomobject->verb),NULL);
sqlite3_bind_text(SQLStatementHandle,8,roomobject->verbmessage,strlen(roomobject->verbmessage),NULL);
sqlite3_bind_int(SQLStatementHandle,9,roomobject->id);

if(sqlite3_step(SQLStatementHandle) != SQLITE_DONE) {
	if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);
	return(-1);
}

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);

return(-1);
}

int LoadWorld(void) {
roomobject *roomobject;
int DirectionCount;
CONFIG config;
sqlite3_stmt *SQLStatementHandle;
int roomreturncode;
char *columnptr;
int id;

GetConfigurationInformation(&config);

NumberOfRooms=1;		/* rooms start from 1 */

/* room list is an array, not a linked list because it is randomly accessed */

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM WORLD;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	printf("mud world: %s\n",sqlite3_errmsg(GetDatabaseHandle()));

	return(-1);	/* prepare SQL statement */
}

if(SQLStatementHandle == NULL) {
	printf("mud world 2: %s\n",sqlite3_errmsg(GetDatabaseHandle()));
	return(-1);
}

rooms=calloc(1,sizeof(room));	/* allocate unused room, room numbers start from 1 */
if(rooms == NULL) {
	perror("mud:");
	return(-1);
}

/* read in the world data */

while(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {
	rooms=realloc(rooms,sizeof(room)*(NumberOfRooms+1));
	if(rooms == NULL) {
		perror("mud:");
		return(-1);
	}

	/* get room information */

	rooms[NumberOfRooms].id=sqlite3_column_int(SQLStatementHandle,ROOM_ID_COLUMN);

	columnptr=sqlite3_column_text(SQLStatementHandle,ROOM_NAME_COLUMN);
	if(columnptr != NULL) strncpy(rooms[NumberOfRooms].name,columnptr,BUF_SIZE);

	columnptr=sqlite3_column_text(SQLStatementHandle,ROOM_OWNER_COLUMN);
	if(columnptr != NULL) strncpy(rooms[NumberOfRooms].owner,columnptr,BUF_SIZE);

	rooms[NumberOfRooms].attributes=sqlite3_column_int(SQLStatementHandle,ROOM_ATTRIBUTES_COLUMN);

	columnptr=sqlite3_column_text(SQLStatementHandle,ROOM_DESCRIPTION_COLUMN);
	strncpy(rooms[NumberOfRooms].description,columnptr,BUF_SIZE);

	rooms[NumberOfRooms].exits[NORTH]=sqlite3_column_int(SQLStatementHandle,ROOM_NORTH_COLUMN);
	rooms[NumberOfRooms].exits[SOUTH]=sqlite3_column_int(SQLStatementHandle,ROOM_SOUTH_COLUMN);
	rooms[NumberOfRooms].exits[EAST]=sqlite3_column_int(SQLStatementHandle,ROOM_EAST_COLUMN);
	rooms[NumberOfRooms].exits[WEST]=sqlite3_column_int(SQLStatementHandle,ROOM_WEST_COLUMN);
	rooms[NumberOfRooms].exits[NORTHEAST]=sqlite3_column_int(SQLStatementHandle,ROOM_NORTHEAST_COLUMN);
	rooms[NumberOfRooms].exits[NORTHWEST]=sqlite3_column_int(SQLStatementHandle,ROOM_NORTHWEST_COLUMN);
	rooms[NumberOfRooms].exits[SOUTHEAST]=sqlite3_column_int(SQLStatementHandle,ROOM_SOUTHEAST_COLUMN);
	rooms[NumberOfRooms].exits[SOUTHWEST]=sqlite3_column_int(SQLStatementHandle,ROOM_SOUTHWEST_COLUMN);
	rooms[NumberOfRooms].exits[UP]=sqlite3_column_int(SQLStatementHandle,ROOM_UP_COLUMN);
	rooms[NumberOfRooms].exits[DOWN]=sqlite3_column_int(SQLStatementHandle,ROOM_DOWN_COLUMN);

	NumberOfRooms++;
}

if(SQLStatementHandle != NULL) sqlite3_finalize(SQLStatementHandle);

return(0);
}

int LoadWorldObjects(void) {
int DirectionCount;
CONFIG config;
sqlite3_stmt *SQLObjectsStatementHandle;
char *columnptr;
int RoomNumber;
int id;

NextObjectNumber=1 << ((sizeof(unsigned int)*8)-1);		/* first object number */

GetConfigurationInformation(&config);

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM WORLDOBJECTS;",-1,&SQLObjectsStatementHandle,NULL) != SQLITE_OK) {
	printf("mud world objects: %s\n",sqlite3_errmsg(GetDatabaseHandle()));

	return(-1);
}

while(sqlite3_step(SQLObjectsStatementHandle) == SQLITE_ROW) {

	/* get room object information */

	id=sqlite3_column_int(SQLObjectsStatementHandle,ROOM_OBJECT_ID_COLUMN);
	RoomNumber=sqlite3_column_int(SQLObjectsStatementHandle,ROOM_OBJECT_ROOM_COLUMN);

	if(RoomNumber > NumberOfRooms) {		/* sanity check */
		printf("mud: Invalid room number for object\n");
		continue;
	}

	if(rooms[RoomNumber].roomobjects == NULL) {	/* first object */
		rooms[RoomNumber].roomobjects=calloc(1,sizeof(roomobject));	/* add first entry */
		if(rooms[RoomNumber].roomobjects == NULL) {
			perror("mud:");
			return(-1);
		}

		rooms[RoomNumber].roomobjects_last=rooms[RoomNumber].roomobjects;
	}
	else
	{
		rooms[RoomNumber].roomobjects_last->next=calloc(1,sizeof(roomobject));	/* add link to end */
		if(rooms[RoomNumber].roomobjects_last->next == NULL) {
			perror("mud:");
			return(-1);
		}

		rooms[RoomNumber].roomobjects_last=rooms[RoomNumber].roomobjects_last->next;
	}

	rooms[RoomNumber].roomobjects_last->id=id;

	columnptr=sqlite3_column_text(SQLObjectsStatementHandle,ROOM_OBJECT_NAME_COLUMN);
	if(columnptr != NULL) strcpy(rooms[RoomNumber].roomobjects_last->name,columnptr);

	columnptr=sqlite3_column_text(SQLObjectsStatementHandle,ROOM_OBJECT_DESCRIPTION_COLUMN);
	if(columnptr != NULL) strcpy(rooms[RoomNumber].roomobjects_last->description,columnptr);

	columnptr=sqlite3_column_text(SQLObjectsStatementHandle,ROOM_OBJECT_OWNER_COLUMN);
	if(columnptr != NULL) strcpy(rooms[RoomNumber].roomobjects_last->owner,columnptr);

	rooms[RoomNumber].roomobjects_last->staminapoints=sqlite3_column_int(SQLObjectsStatementHandle,ROOM_OBJECT_STAMINA_POINTS_COLUMN);
	rooms[RoomNumber].roomobjects_last->magicpoints=sqlite3_column_int(SQLObjectsStatementHandle,ROOM_OBJECT_MAGIC_POINTS_COLUMN);
	rooms[RoomNumber].roomobjects_last->attributes=sqlite3_column_int(SQLObjectsStatementHandle,ROOM_OBJECT_ATTRIBUTES_COLUMN);

	columnptr=sqlite3_column_text(SQLObjectsStatementHandle,ROOM_OBJECT_VERB_COLUMN);
	if(columnptr != NULL) strcpy(rooms[RoomNumber].roomobjects_last->verb,columnptr);

	columnptr=sqlite3_column_text(SQLObjectsStatementHandle,ROOM_OBJECT_VERBMESSAGE_COLUMN);
	if(columnptr != NULL) strcpy(rooms[RoomNumber].roomobjects_last->verbmessage,columnptr);
}

if(SQLObjectsStatementHandle != NULL) sqlite3_finalize(SQLObjectsStatementHandle);

return(0);
}

int SetExit(user *currentuser,int whichroom,char *setexitdirection,int targetroom) {
int count;

if(currentuser->userlevel < WIZARD) {	/* can't do this yet */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

if((whichroom > GetNumberOfRooms()) || (targetroom > GetNumberOfRooms())) {
	SetLastError(currentuser,ROOM_NOT_FOUND);  
	return(-1);
}

for(count=0;count < 10;count++) {
	if(strncmp(setexitdirection,directions,BUF_SIZE) == 0) {
		rooms[whichroom].exits[count]=targetroom;
		
		return(0);
	}
}

SetLastError(currentuser,INVALID_EXIT);
return(-1);
}

/* create room */

int CreateRoom(user *currentuser,char *roomdirection) {
int LastRoom;
int RoomDirectionCount;
char *CreateMessage[BUF_SIZE];
sqlite3_stmt *SQLStatementHandle;

if(currentuser->userlevel < WIZARD) {	/* must be wizard or higher level to create room */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

/* deny access if not owner and not allowed to create exit */

if(currentuser->userlevel < ARCHWIZARD) {
	if((strncmp(rooms[currentuser->room].owner,currentuser->username,BUF_SIZE) != 0) && (rooms[currentuser->room].attributes  & ROOM_EXIT_PUBLIC) == 0 && currentuser->userlevel < ARCHWIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	/* deny permission if not owner not allowed to create objects in room */

	if((strncmp(rooms[currentuser->room].owner,currentuser->username,BUF_SIZE) == 0) && (rooms[currentuser->room].attributes  & ROOM_EXIT_OWNER) == 0 && currentuser->userlevel < ARCHWIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}
}

/* if the room is in argument, create it */

if(*roomdirection) {		/* room spcified */
	for(RoomDirectionCount=0;RoomDirectionCount != 11;RoomDirectionCount++) {
		if(strncmp(roomdirection,directions[RoomDirectionCount],BUF_SIZE) == 0) break;	/* found */
	}
}
else
{
	for(RoomDirectionCount=0;RoomDirectionCount != 11;RoomDirectionCount++) {
		if(rooms[currentuser->room].exits[RoomDirectionCount] == 0) break;	/* found */
	}
}

if(RoomDirectionCount > 11) { 
	SetLastError(currentuser,INVALID_DIRECTION);  
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
strncpy(rooms[LastRoom].name,"Empty room\r\r\n",BUF_SIZE);	/* create room info */
strncpy(rooms[LastRoom].owner,currentuser->username,BUF_SIZE);
strncpy(rooms[LastRoom].description,"Empty room, you can describe it using desc here <description>\r\n",BUF_SIZE);

rooms[LastRoom].attributes=ROOM_CREATE_OWNER | ROOM_EXIT_OWNER | ROOM_RENAME_OWNER;
rooms[LastRoom].id=LastRoom;
rooms[LastRoom].roomobjects=NULL;

currentuser->roomptr->exits[RoomDirectionCount]=GetNextObjectNumber();
	
/* insert into database */

/* prepare statement SQL */

if(sqlite3_prepare_v2(GetDatabaseHandle(),"INSERT INTO WORLD COLUMNS (ID,NAME,OWNER,DESC,attributes,NORTH,SOUTH,EAST,WEST,NORTHEAST,NORTHWEST,SOUTHEAST,SOUTHWEST,UP,DOWN) VALUES  (?,?,?,?,?,?,?,?,?,?, ?,?,?,?,?);",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

/* bind parameters */
sqlite3_bind_int(SQLStatementHandle,1,rooms[LastRoom].id);
sqlite3_bind_text(SQLStatementHandle,1,rooms[LastRoom].name,strlen(rooms[LastRoom].name),NULL);
sqlite3_bind_text(SQLStatementHandle,2,rooms[LastRoom].owner,strlen(rooms[LastRoom].owner),NULL);
sqlite3_bind_text(SQLStatementHandle,4,rooms[LastRoom].description,strlen(rooms[LastRoom].description),NULL);
sqlite3_bind_int(SQLStatementHandle,3,rooms[LastRoom].attributes);
sqlite3_bind_int(SQLStatementHandle,5,rooms[LastRoom].exits[NORTH]);
sqlite3_bind_int(SQLStatementHandle,6,rooms[LastRoom].exits[SOUTH]);
sqlite3_bind_int(SQLStatementHandle,7,rooms[LastRoom].exits[EAST]);
sqlite3_bind_int(SQLStatementHandle,8,rooms[LastRoom].exits[WEST]);
sqlite3_bind_int(SQLStatementHandle,9,rooms[LastRoom].exits[NORTHEAST]);
sqlite3_bind_int(SQLStatementHandle,10,rooms[LastRoom].exits[NORTHWEST]);
sqlite3_bind_int(SQLStatementHandle,11,rooms[LastRoom].exits[SOUTHEAST]);
sqlite3_bind_int(SQLStatementHandle,12,rooms[LastRoom].exits[SOUTHWEST]);
sqlite3_bind_int(SQLStatementHandle,13,rooms[LastRoom].exits[UP]);
sqlite3_bind_int(SQLStatementHandle,14,rooms[LastRoom].exits[DOWN]);

if(sqlite3_step(SQLStatementHandle) != SQLITE_DONE) {
	sqlite3_finalize(SQLStatementHandle);
	return(-1);
}

SetLastError(currentuser,NO_ERROR);
return(0);
}

/* set an object, room or user's attributes */

int SetObjectAttributes(user *currentuser,unsigned int ObjectID,int attributes) {
roomobject *ObjectPtr;
int RoomCount;
char *ErrorMessage[BUF_SIZE];


if(currentuser->userlevel < WIZARD) {            /* need to be wizard or higher level to change attributes */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

ObjectPtr=currentuser->roomptr->roomobjects;	/* point to objects */

while(ObjectPtr != NULL) {

	if(ObjectPtr->id == ObjectID) {	/* found object */
		if(currentuser->userlevel < ARCHWIZARD) {
			if((strncmp(ObjectPtr->owner,currentuser->username,BUF_SIZE) == 0) && (ObjectPtr->attributes & OBJECT_MOVEABLE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
		}

		if((strncmp(ObjectPtr->owner,currentuser->username,BUF_SIZE) == 0) && (ObjectPtr->attributes & OBJECT_MOVEABLE_OWNER) == 0) {
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
		}
	}

	ObjectPtr->attributes=attributes;

	UpdateObjectInDatabase(currentuser,ObjectPtr);

	SetLastError(currentuser,NO_ERROR);
	return(0);
	}


	ObjectPtr=ObjectPtr->next;
}


/* Setting a room's attribute */

rooms[RoomCount].attributes=attributes;

SetLastError(currentuser,NO_ERROR);
return(0);
}


/* set owner of room or object */

int SetOwner(user *currentuser,unsigned int ObjectID,char *owner) {
room *roomnext;
roomobject *ObjectPtr;
int RoomCount;


if(currentuser->userlevel < WIZARD) {             /* can't do this unless wizard or higher level */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}
	
if(currentuser->userlevel <ARCHWIZARD) {

	if(strncmp(owner,currentuser->username,BUF_SIZE) == 0) {                    /* unless archwizard, or higher, must be object's owner to modify */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
	} 
}

/* find object */

ObjectPtr=currentuser->roomptr->roomobjects;
while(ObjectPtr != NULL) {

	if(ObjectPtr->id == ObjectID) { 		/* if object found */
		strncpy(ObjectPtr->owner,owner,BUF_SIZE);

		UpdateObjectInDatabase(currentuser,ObjectPtr);
		return(0);
	}

	ObjectPtr=ObjectPtr->next;
}

/*  find room */

for(RoomCount=0;RoomCount < GetNumberOfRooms();RoomCount++) {
	if(rooms[RoomCount].id == ObjectID) {	/* if object found */
		if(currentuser->userlevel < ARCHWIZARD && strncmp(rooms[RoomCount].owner,currentuser->username,BUF_SIZE) != 0) {	/* permission denied */
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
		}

		strncpy(rooms[RoomCount].owner,owner,BUF_SIZE);   

		UpdateRoomInDatabase(currentuser,RoomCount);       		/* update database */
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


if(currentuser->userlevel < WIZARD) {      /* not wizard */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

if(GetNumberOfRooms() > currentuser->roomptr->id) {		/* can't find room */
	SetLastError(currentuser,OBJECT_NOT_FOUND);  
	return(-1);
}

ObjectPtr=currentuser->roomptr->roomobjects;

while(ObjectPtr != NULL) {
	if(ObjectPtr->id == ObjectID) {	/* if object matches */

		if(currentuser->userlevel < ARCHWIZARD) {
			if((strncmp(ObjectPtr->owner,currentuser->username,BUF_SIZE) == 0) && (ObjectPtr->attributes & OBJECT_MOVEABLE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		if((strncmp(ObjectPtr->owner,currentuser->username,BUF_SIZE) == 0) && (ObjectPtr->attributes & OBJECT_MOVEABLE_OWNER) == 0) {
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
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

		// XXX
		UpdateObjectInDatabase(currentuser,ObjectPtr);

		found=TRUE;
	}

	ObjectPtr=ObjectPtr->next;
}

SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}

/* generate objects */

int GenerateObjects(void) {
int NumberOfObjectsToGenerate=0;
int RandomObjectNumber;
int RoomCount=0;
CONFIG config;
sqlite3_stmt *SQLStatementHandle;
int returncode;
roomobject *ObjectPtr;

GetConfigurationInformation(&config);

/* Generate room objects */

for(RoomCount=1;RoomCount < GetNumberOfRooms(); RoomCount++) {
//	if((rooms[RoomCount].attributes & ROOM_HAVEN) == 0) {          /* not haven */

		NumberOfObjectsToGenerate=rand() % (config.MaximumNumberOfObjectsPerRoom + 1);

		if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT NAME,DESCRIPTION,ATTACKPOINTS,GENERATEPROB,STAMINAPOINTS,MAGICPOINTS,ATTRIBUTES,VERB,VERBMESSAGE FROM GENERATABLEOBJECTS ORDER BY RANDOM() LIMIT ?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	/* prepare SQL statement */
			printf("mud: %s\n",sqlite3_errmsg(GetDatabaseHandle()));

			return(-1);
		}	

		if(SQLStatementHandle == NULL) {
			printf("mud: %s\n",sqlite3_errmsg(GetDatabaseHandle()));
			
			return(-1);
		}

		sqlite3_bind_int(SQLStatementHandle,1,NumberOfObjectsToGenerate);	/* bind parameter */
		
		while(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {

			if(rooms[RoomCount].roomobjects == NULL) {	/* no objects */
				rooms[RoomCount].roomobjects=calloc(1,sizeof(roomobject));	/* add link to end */
				if(rooms[RoomCount].roomobjects == NULL) return(-1);	/* can't allocate */

				rooms[RoomCount].roomobjects_last=rooms[RoomCount].roomobjects;

				ObjectPtr->prev=NULL;
				ObjectPtr->next=NULL;
			}
			else
			{
				rooms[RoomCount].roomobjects_last->next=calloc(1,sizeof(roomobject));
				if(rooms[RoomCount].roomobjects_last->next == NULL) return(-1);

				rooms[RoomCount].roomobjects_last->next->prev=rooms[RoomCount].roomobjects_last;	/* point to previous */
				rooms[RoomCount].roomobjects_last=rooms[RoomCount].roomobjects_last->next;
			}

			strncpy(rooms[RoomCount].roomobjects_last->name,sqlite3_column_text(SQLStatementHandle,GENERATABLE_OBJECT_NAME_COLUMN),BUF_SIZE);
			strncpy(rooms[RoomCount].roomobjects_last->owner,"nobody",BUF_SIZE);
			strncpy(rooms[RoomCount].roomobjects_last->description,sqlite3_column_text(SQLStatementHandle,GENERATABLE_OBJECT_DESCRIPTION_COLUMN),BUF_SIZE);
			rooms[RoomCount].roomobjects_last->attackpoints=sqlite3_column_int(SQLStatementHandle,GENERATABLE_OBJECT_ATTACK_POINTS_COLUMN);
			rooms[RoomCount].roomobjects_last->generateprobability=sqlite3_column_int(SQLStatementHandle,GENERATABLE_OBJECT_GENERATE_PROBABILITY_COLUMN);
			rooms[RoomCount].roomobjects_last->staminapoints=sqlite3_column_int(SQLStatementHandle,GENERATABLE_OBJECT_STAMINA_POINTS_COLUMN);
			rooms[RoomCount].roomobjects_last->magicpoints=sqlite3_column_int(SQLStatementHandle,GENERATABLE_OBJECT_MAGIC_POINTS_COLUMN);
			strncpy(rooms[RoomCount].roomobjects_last->verb,sqlite3_column_text(SQLStatementHandle,GENERATABLE_OBJECT_VERB_COLUMN),BUF_SIZE);
			strncpy(rooms[RoomCount].roomobjects_last->verbmessage,sqlite3_column_text(SQLStatementHandle,GENERATABLE_OBJECT_VERBMESSAGE_COLUMN),BUF_SIZE);
		}
//	}
}

sqlite3_finalize(SQLStatementHandle);
return(0);
}

/* create object */

int CreateObject(user *currentuser,roomobject *object,int room) {
roomobject *ObjectPtr;
roomobject *ObjPtrLast;
char *buf[BUF_SIZE];
sqlite3_stmt *SQLStatementHandle;

if(currentuser->userlevel < WIZARD) {         /* can't create object unless wizard level or above */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

/* deny permission if not archwizard and the owner doesn't allow others to create objects in room */

if(currentuser->userlevel < ARCHWIZARD) {
	if((strncmp(currentuser->roomptr->owner,currentuser->username,BUF_SIZE) != 0) && (currentuser->roomptr->attributes  & ROOM_CREATE_PUBLIC) == 0) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

/* deny permission if not archwizard and owner not allowed to create objects in room */

	if(((strncmp(currentuser->roomptr->owner,currentuser->username,BUF_SIZE) == 0) && (currentuser->roomptr->attributes  & ROOM_CREATE_OWNER) == 0)) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}
}

if(currentuser->roomptr->roomobjects == NULL) {	/* no objects */
	currentuser->roomptr->roomobjects=calloc(1,sizeof(roomobject));	/* add link to end */
	if(currentuser->roomptr->roomobjects == NULL) {	/* can't allocate */
		SetLastError(currentuser,NO_MEM);  
		return(-1);
	}

	ObjectPtr=currentuser->roomptr->roomobjects;
	ObjectPtr->prev=NULL;
	ObjectPtr->next=NULL;
}
else
{
	ObjectPtr=currentuser->roomptr->roomobjects_last;

	ObjectPtr->next=calloc(1,sizeof(roomobject));
	if(ObjectPtr == NULL) { /* can't allocate */
		SetLastError(currentuser,NO_MEM);  
		return(-1);
	}

	ObjectPtr->next->prev=ObjectPtr;		/* point to previous */
	ObjectPtr=ObjectPtr->next;
}

memcpy(ObjectPtr,object,sizeof(roomobject));		/* add object details at end */

ObjectPtr->id=GetNextObjectNumber();	/* generate new ID number for created object */
ObjectPtr->attributes=OBJECT_DELETE_OWNER | OBJECT_MOVEABLE_OWNER | OBJECT_PICKUP_OWNER | OBJECT_RENAME_OWNER;
ObjectPtr->next=NULL;

/* update database */

if(sqlite3_prepare_v2(GetDatabaseHandle(),"INSERT INTO WORLDOBJECTS (ID,ROOM,NAME,DESCRIPTION,OWNER,STAMINAPOINTS,MAGICPOINTS,ATTRIBUTES,VERB,VERBMESSAGE) VALUES (?,?,?,?,?,?,?,?,?,?);",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
	printf("mud generate objects: %s\n",sqlite3_errmsg(GetDatabaseHandle()));

	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

/* bind parameters */
sqlite3_bind_int(SQLStatementHandle,1,ObjectPtr->id);
sqlite3_bind_int(SQLStatementHandle,2,room);
sqlite3_bind_text(SQLStatementHandle,3,ObjectPtr->name,strlen(ObjectPtr->name),NULL);
sqlite3_bind_text(SQLStatementHandle,4,ObjectPtr->description,strlen(ObjectPtr->description),NULL);
sqlite3_bind_text(SQLStatementHandle,5,ObjectPtr->owner,strlen(ObjectPtr->owner	),NULL);
sqlite3_bind_int(SQLStatementHandle,6,ObjectPtr->staminapoints);
sqlite3_bind_int(SQLStatementHandle,7,ObjectPtr->magicpoints);
sqlite3_bind_int(SQLStatementHandle,8,ObjectPtr->attributes);
sqlite3_bind_text(SQLStatementHandle,9,ObjectPtr->verb,strlen(ObjectPtr->verb),NULL);
sqlite3_bind_text(SQLStatementHandle,10,ObjectPtr->verbmessage,strlen(ObjectPtr->verbmessage),NULL);

if(sqlite3_step(SQLStatementHandle) != SQLITE_DONE) {
	sqlite3_finalize(SQLStatementHandle);
	return(-1);
}

sqlite3_finalize(SQLStatementHandle);

return(0);
}

/* delete object */

int DeleteObject(user *currentuser,unsigned int ObjectID) {
roomobject *ObjectPtr;
roomobject *OldPtr;
sqlite3_stmt *SQLStatementHandle;

if(currentuser->userlevel < WIZARD) {             /* can't do this unless wizard or higher level */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

ObjectPtr=currentuser->roomptr->roomobjects;

while(ObjectPtr != NULL) {
	if(ObjectPtr->id == ObjectID) {	/* found */
		/* can't delete if not owner and OBJECT_DELETE_PUBLIC attribute not set */

		if(currentuser->userlevel < ARCHWIZARD) {
			if((strncmp(ObjectPtr->owner,currentuser->username,BUF_SIZE) != 0) && (ObjectPtr->attributes & OBJECT_DELETE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			/* object owner and OBJECT_MOVEABLE_OWNER attribute not set, display error message */

			if((strncmp(ObjectPtr->owner,currentuser->username,BUF_SIZE) == 0) && (ObjectPtr->attributes & OBJECT_DELETE_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		/* remove object from room */

		if(ObjectPtr == currentuser->roomptr->roomobjects) {	/* first object */
			OldPtr=currentuser->roomptr->roomobjects;

			currentuser->roomptr->roomobjects=currentuser->roomptr->roomobjects->next;
		}
		else if(ObjectPtr->next == NULL) {	/* last object */
			OldPtr=ObjectPtr;

			ObjectPtr->prev->next=NULL;	/* new end of list */
		}
		else {
			OldPtr=ObjectPtr;

			ObjectPtr->prev->next=ObjectPtr->next;
		}

		free(OldPtr);

		/* update database */
	
		if(sqlite3_prepare_v2(GetDatabaseHandle(),"DELETE FROM WORLDOBJECTS WHERE ID=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {
			SetLastError(currentuser,IO_ERROR);
			return(-1);
		}

		sqlite3_bind_int(SQLStatementHandle,1,ObjectID);

		sqlite3_step(SQLStatementHandle);
	
		sqlite3_finalize(SQLStatementHandle);
		return(0);
	}

	ObjectPtr=ObjectPtr->next;
}

SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}

int RenameObject(user *currentuser,unsigned int ObjectID,char *newname) {
user *usernext;
roomobject *ObjectPtr;
 
if(currentuser->userlevel < WIZARD) {	/* can't do this yet */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

/* renaming object */
ObjectPtr=currentuser->roomptr->roomobjects;

while(ObjectPtr->next != NULL) {

	if(ObjectPtr->id == ObjectID) {	/* found object */
		/* check permissions */

		if(currentuser->userlevel < ARCHWIZARD) {	/* is owner but can't rename */

			if((strncmp(ObjectPtr->owner,currentuser->next,BUF_SIZE) == 0) && ((ObjectPtr->attributes & OBJECT_RENAME_OWNER) == 0)) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			/* not owner and can't rename */

			if((strncmp(ObjectPtr->owner,currentuser->next,BUF_SIZE) != 0) && ((ObjectPtr->attributes & OBJECT_RENAME_PUBLIC) == 0)) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		strncpy(ObjectPtr->name,newname,BUF_SIZE);	/* rename object */

		UpdateObjectInDatabase(currentuser,ObjectPtr->id);
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

		if(currentuser->userlevel < ARCHWIZARD) {
			if((strncmp(roomobject->owner,currentuser->username,BUF_SIZE) == 0) && (roomobject->attributes & OBJECT_RENAME_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			/* if not owner and OBJECT_RENAME_PUBLIC attribute not set, then display an error message */

			if((strncmp(roomobject->owner,currentuser->username,BUF_SIZE) != 0) && (roomobject->attributes & OBJECT_RENAME_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}


		strncpy(roomobject->description,description,BUF_SIZE);               /* set object description	*/
		UpdateObjectInDatabase(currentuser,roomobject);                     /* update database */

		SetLastError(currentuser,NO_ERROR);
		return(0);
	}

	roomobject=roomobject->next;
}


SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
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
return(rooms[RoomNumber].attributes);
}

void SetFirstMonsterInRoom(int RoomNumber,monster *Monster) {
rooms[RoomNumber].roommonsters=Monster;
}

int IsObjectInRoom(char *name,int RoomNumber) {
roomobject *roomptr;

roomptr=rooms[RoomNumber].roomobjects;

/* search through list of objects */

while(roomptr != NULL) {
	if(strncmp(name,roomptr->name,BUF_SIZE) == 0) return(TRUE);	/* found object */

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

strncpy(rooms[RoomNumber].description,description,BUF_SIZE);	/* set description */

if(UpdateRoomInDatabase(currentuser,RoomNumber) == -1) return(-1);

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

	if(strncmp(ObjectPtr->name,ObjectName,BUF_SIZE) == 0) {		/* found object */
		printf("Found object\n");

		printf("verb=%s ** %s\n",verb,ObjectPtr->verb);

		if(strncmp(verb,ObjectPtr->verb,BUF_SIZE) == 0) {	/* found verb */	
			printf("Found verb\n");

			send(currentuser->socket,ObjectPtr->verbmessage,strlen(ObjectPtr->verbmessage),0);

			if(ObjectPtr->attributes & OBJECT_TEMPORARY) {
				if(DeleteObject(currentuser,ObjectPtr->id) == -1) return(-1);
			}

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

