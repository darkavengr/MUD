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

char *DatabaseConfigurationFile="config/database.mud";                                          /* configuration files */
int DatabaseUpdated;
char *directions[]={ "north","south","east","west","northeast","northwest","southeast","southwest","up","down" };
char *ObjectsConfigurationFile="/config/reset.mud";
char *RoomMessage="\r\nIn the room there is: ";
char *RoomExits[]={ "North ","South ","East ","West ","Northeast ","Northwest ","Southeast ","Southwest ","Up ","Down " };
room *rooms=NULL;
int NumberOfRooms=0;

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

time(&rawtime);				/* get time for backup filename */
timeinfo=localtime(&rawtime);

/* if configured to backup, copy database to backup file */

if(config.databasebackup == TRUE) {
	strftime(NewDatabaseName,BUF_SIZE,"config/database-%H-%M-%S-%d-%e.sav",timeinfo);	/* get backup name */

	sprintf(BackupFilename,"%s/%s",CurrentDirectory,NewDatabaseName);			/* get path of backup */

	CopyFile(DatabaseConfigurationFile,BackupFilename);		/* backup database */
}

handle=fopen(DatabaseConfigurationFile,"w");
if(handle == NULL) return(-1);	/* can't open file */

/*
* for each room output to file
*/
for(RoomCount=0;RoomCount<GetNumberOfRooms();RoomCount++) {
	if(rooms[RoomCount].room > 0) {
		fprintf(handle,"begin_room:%d\n",rooms[RoomCount].room);
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

int LoadDatabase(void) {
FILE *handle;
int LineCount=0;
roomobject *roomobject;
char *LineBuffer[BUF_SIZE];
char *LineTokens[BUF_SIZE][BUF_SIZE];
int DirectionCount;
int ErrorCount=0;
CONFIG config;
char *CurrentDirectory[BUF_SIZE];

GetConfigurationInformation(&config);

/* get path of database */

handle=fopen(DatabaseConfigurationFile,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",DatabaseConfigurationFile);
	exit(NOCONFIGFILE);
}

NumberOfRooms=0;

	while(!feof(handle)) {
		fgets(LineBuffer,BUF_SIZE,handle);				/*' get and parse line */
	
		if((char) *LineBuffer == '#')  continue;		/* skip comments */
		if((char) *LineBuffer == '\n')  continue;		/* skip newline */
	
		RemoveNewLine(LineBuffer);		/* remove newline character */

		TokenizeLine(LineBuffer,LineTokens,":\n");				/* tokenize line */

		LineCount++;

		if(strcmp(LineTokens[0],"begin_room") == 0) {		/* room name */
			/* room list is an array, not a linked list because it is randomly accessed */

			if(rooms == NULL) {			/* first room */
				rooms=calloc(1,sizeof(room));
				if(rooms == NULL) {
					perror("\nmud:");
					exit(NOMEM);
				}
			}
			else
			{
				NumberOfRooms++;

				rooms=realloc(rooms,sizeof(room)*(NumberOfRooms+1));
				if(rooms == NULL) {
					perror("\nmud:");
					exit(NOMEM);
				}
			}

			rooms[NumberOfRooms].room=atoi(LineTokens[1]);
			
			continue;
		}
		else if(strcmp(LineTokens[0],"name") == 0) {		/* room name */
			strcpy(rooms[NumberOfRooms].name,LineTokens[1]);
			continue;
		}
		else if(strcmp(LineTokens[0],"owner") == 0) {		/* owner */
			strcpy(rooms[NumberOfRooms].owner,LineTokens[1]);
			continue;
		}
		else if(strcmp(LineTokens[0],"attr") == 0) {		/* attributes */
			rooms[NumberOfRooms].attr=atoi(LineTokens[1]);
			continue;
		}
		else if(strcmp(LineTokens[0],"description") == 0) {		/* description */
			strcpy(rooms[NumberOfRooms].desc,LineTokens[1]);
			continue;
		}
		else if(strcmp(LineTokens[0],"name") == 0) {
			strcpy(rooms[NumberOfRooms].name,LineTokens[1]);
			continue;
		}
		else if(strcmp(LineTokens[0],"object") == 0) {		/* room object */

			if(rooms[NumberOfRooms].roomobjects == NULL) {		/* first object */
				rooms[NumberOfRooms].roomobjects=calloc(1,sizeof(roomobject));		/* add link to end */

				if(rooms[NumberOfRooms].roomobjects == NULL) {		/* can't allocate */
					perror("mud:");
					exit(NOMEM);
				}

				roomobject=rooms[NumberOfRooms].roomobjects;
				rooms[NumberOfRooms].roomobjects_last=rooms[NumberOfRooms].roomobjects;
			}
			else
			{
				rooms[NumberOfRooms].roomobjects_last->next=calloc(1,sizeof(roomobject));		/* add link to end */
				if(rooms[NumberOfRooms].roomobjects_last->next == NULL) {		/* can't allocate */
					perror("mud:");
					exit(NOMEM);
				}

				rooms[NumberOfRooms].roomobjects_last=rooms[NumberOfRooms].roomobjects_last->next;
				roomobject=rooms[NumberOfRooms].roomobjects_last;
			}

			strcpy(roomobject->name,LineTokens[OBJECT_NAME]);		/* get details */
			roomobject->attr=atoi(LineTokens[OBJECT_ATTR]);
			strcpy(roomobject->desc,LineTokens[OBJECT_DESCRIPTION]);		
			roomobject->attackpoints=atoi(LineTokens[OBJECT_ATTACKPOINTS]);		
			roomobject->staminapoints=atoi(LineTokens[OBJECT_GENERATEPROB]);
			roomobject->magicpoints=atoi(LineTokens[OBJECT_MAGICPOINTS]);   
			roomobject->next=NULL;
			continue;
		}
		else if(strcmp(LineTokens[0],"end") == 0) {
			continue;
		}

		/* handle room exits */

		for(DirectionCount=0;DirectionCount<12;DirectionCount++) {
			if(strcmp(LineTokens[0],directions[DirectionCount]) == 0) {		/* room exits */
				rooms[NumberOfRooms].exits[DirectionCount]=atoi(LineTokens[1]);
				break;
			}

			continue;
		}

		if(DirectionCount < 12) continue;		/* was a north, south, east, west,... statement */

		if((char) *LineTokens[0] == '#') continue;			/* comment */

		printf("\nmud: %d: Unknown configuration option %s in %s\n",LineCount,LineTokens[0],DatabaseConfigurationFile);		/* unknown configuration option */
		ErrorCount++;
		continue;
	}

fclose(handle);
return(ErrorCount);
}

int SetExit(user *currentuser,int whichroom,int direction,int exit) {
if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(direction > 11) {
	SetLastError(currentuser,INVALID_EXIT);
	return(-1);
}

if(direction > GetNumberOfRooms()) {
	SetLastError(currentuser,BAD_ROOM);  
	return(-1);
}

rooms[whichroom].exits[direction]=exit;
return(0);
}

/* create room */

int CreateRoom(user *currentuser,char *roomdirection) {
int LastRoom;
room *CurrentRoom;
int RoomDirectionCount;
char *CreateMessage[BUF_SIZE];

CurrentRoom=currentuser->roomptr;

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


/* if the room is in argument, create it */

if(*roomdirection) {			/* room spcified */
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

rooms=realloc(rooms,(sizeof(room)*LastRoom)+1);		/* resize database */
if(rooms == NULL) {
	SetLastError(currentuser,NO_MEM);  
	return(-1);
}

memset(&rooms[LastRoom],0,sizeof(room));		/* clear room */
strcpy(rooms[LastRoom].name,"Empty room\r\r\n");		/* create room info */
strcpy(rooms[LastRoom].owner,currentuser->name);
strcpy(rooms[LastRoom].desc,"Empty room, you can describe it using desc here <description>\r\n");

rooms[LastRoom].attr=ROOM_CREATE_OWNER | ROOM_EXIT_OWNER | ROOM_RENAME_OWNER;
rooms[LastRoom].room=LastRoom;

CurrentRoom->exits[RoomDirectionCount]=GetNumberOfRooms();

sprintf(CreateMessage,"A room has been created to the %s\r\n",RoomExits[RoomDirectionCount]);
send(currentuser->handle,CreateMessage,strlen(CreateMessage),0);

DatabaseUpdated=TRUE;                  /* update database flag */
return(0);
}

/* set an object, room or user's attributes */

int SetObjectAttributes(user *currentuser,char *object,char *attributes) {
roomobject *ObjectPtr;
room *CurrentRoom;
int RoomCount;
int ObjectFound=FALSE;
char *ErrorMessage[BUF_SIZE];
char *AttributePtr;

CurrentRoom=currentuser->roomptr;

if(currentuser->status < WIZARD) {            /* need to be wizard or higher level to change attributes */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

ObjectPtr=CurrentRoom->roomobjects;		/* point to objects */

while(ObjectPtr != NULL) {

	if(regexp(ObjectPtr->name,object) == TRUE) {		/* found object */
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

		AttributePtr=attributes;

		while(*AttributePtr != 0) {
			if(*AttributePtr == '+') {	/* setting attribute */

				AttributePtr++;

				switch(*AttributePtr++) {
					case 'd':
						ObjectPtr->attr |= OBJECT_DELETE_OWNER;
				  		 break;

					case 'D':
				   		ObjectPtr->attr |= OBJECT_DELETE_PUBLIC;
				   		break;

					case 'm':
				   		ObjectPtr->attr |= OBJECT_MOVEABLE_OWNER;
				   		break;
		
					case 'M':
						ObjectPtr->attr |= OBJECT_MOVEABLE_PUBLIC;
						break;
		
					case 'p':
						ObjectPtr->attr |= OBJECT_PICKUP_OWNER;
						break;

					case 'P':
				   		ObjectPtr->attr |= OBJECT_PICKUP_PUBLIC;
				   		break;
		
					case 'r':
						ObjectPtr->attr |= OBJECT_RENAME_OWNER;
						break;
		
					case 'R':
						ObjectPtr->attr |= OBJECT_RENAME_PUBLIC;
						break;
		
				 	case 't':
				   		ObjectPtr->attr |= OBJECT_TEMPORARY;
				   		break;
		
					default:
						sprintf(ErrorMessage,"Unknown attribute %c\r\n",*AttributePtr);
						send(currentuser->handle,ErrorMessage,strlen(ErrorMessage),0);
					}
				}
			}

			if(*AttributePtr == '-') {
				AttributePtr++;

				switch(*AttributePtr++) {
					case 'd':
						ObjectPtr->attr &= OBJECT_DELETE_OWNER;
						break;

					case 'D':
						ObjectPtr->attr &= OBJECT_DELETE_PUBLIC;
						break;

				  	case 'm':
				   		ObjectPtr->attr &= OBJECT_MOVEABLE_OWNER;
				   		break;
		
				  	case 'M':
				   		ObjectPtr->attr &= OBJECT_MOVEABLE_PUBLIC;
				   		break;
		
				 	case 'p':
				   		ObjectPtr->attr &= OBJECT_PICKUP_OWNER;
				   		break;

				 	case 'P':
				   		ObjectPtr->attr &= OBJECT_PICKUP_PUBLIC;
				   		break;
		
				 	case 'r':
						ObjectPtr->attr &= OBJECT_RENAME_OWNER;
						break;
		
				 	case 'R':
				   		ObjectPtr->attr &= OBJECT_RENAME_PUBLIC;
				   		break;

				 	case 't':
				   		ObjectPtr->attr &= OBJECT_TEMPORARY;
				   		break;
		
					default:
				  		sprintf(ErrorMessage,"Unknown attribute %c\r\n",*AttributePtr);
						send(currentuser->handle,ErrorMessage,strlen(ErrorMessage),0);
					}
			} 

			ObjectFound=TRUE;
		}


	ObjectPtr=ObjectPtr->next;
}


/* Setting a room's attribute */

for(RoomCount=0;RoomCount<GetNumberOfRooms();RoomCount++) {

	if(regexp(rooms[RoomCount].name,object) == TRUE) {		/* found room */
		AttributePtr=attributes;

		if(*AttributePtr == '+') {	/* setting attribute */
			AttributePtr++;

			switch(*AttributePtr++) {
			  	case 'c':
					rooms[RoomCount].attr |= ROOM_CREATE_OWNER;
					break;

				case 'C':
					rooms[RoomCount].attr |= ROOM_CREATE_PUBLIC;
					break;

				case 'e':
					rooms[RoomCount].attr |= ROOM_EXIT_OWNER;
					break;

				case 'E':
					rooms[RoomCount].attr |= ROOM_CREATE_PUBLIC;
					break;
	
				case 'r':
					rooms[RoomCount].attr |= ROOM_RENAME_OWNER;
					break;

				case 'R':
					rooms[RoomCount].attr |= ROOM_RENAME_PUBLIC;
					break;

				case 'h':
					rooms[RoomCount].attr |= ROOM_HAVEN;
					break;

				case 'p':
					rooms[RoomCount].attr |= ROOM_PRIVATE;
					break;

				default:
					sprintf(ErrorMessage,"Unknown attribute %c\r\n",*AttributePtr);
					send(currentuser->handle,ErrorMessage,strlen(ErrorMessage),0);
				}
				
		}
	
		if(*AttributePtr == '-') {
			switch(*AttributePtr++) {
			 	case 'c':
					rooms[RoomCount].attr &= ROOM_CREATE_OWNER;
					break;

			 	case 'C':
					rooms[RoomCount].attr &= ROOM_CREATE_PUBLIC;
					break;

				case 'e':
					rooms[RoomCount].attr &= ROOM_EXIT_OWNER;
					break;

			 	case 'E':
					rooms[RoomCount].attr &= ROOM_EXIT_PUBLIC;
					break;
	
			 	case 'r':
					rooms[RoomCount].attr &= ROOM_RENAME_OWNER;
					break;

				 case 'R':
					rooms[RoomCount].attr &= ROOM_RENAME_PUBLIC;
					break;

			 	case 'h':
					rooms[RoomCount].attr &= ROOM_HAVEN;
					break;

			 	case 'p':
					rooms[RoomCount].attr &= ROOM_PRIVATE;
					break;

				default:
					sprintf(ErrorMessage,"Unknown attribute %c\r\n",*AttributePtr);
					send(currentuser->handle,ErrorMessage,strlen(ErrorMessage),0);
				}
			}

		ObjectFound=TRUE;
	}
}

if(ObjectFound == FALSE)  {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  
	return(-1);
}

SetLastError(currentuser,NO_ERROR);
return(0);
}


/* set owner of room or object */

int SetOwner(user *currentuser,char *objectname,char *owner) {
room *roomnext;
roomobject *ObjectPtr;
room *CurrentRoom;
int RoomCount;

CurrentRoom=currentuser->roomptr;

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

/*
* find object
*/

ObjectPtr=CurrentRoom->roomobjects;
while(ObjectPtr != NULL) {

	if(regexp(ObjectPtr->name,objectname) == TRUE) { 			/* if object found */
		strcpy(ObjectPtr->owner,owner);
		DatabaseUpdated=TRUE;

		return(0);
	}

	ObjectPtr=ObjectPtr->next;
}

/*
* not object, find room to change owner
*/

for(RoomCount=0;RoomCount<GetNumberOfRooms();RoomCount++) {
	if(rooms[RoomCount].room == atoi(objectname)) {		/* if object found */

		if(currentuser->status < ARCHWIZARD && strcmp(rooms[RoomCount].owner,currentuser->name) != 0) {	/* permission denied */
			SetLastError(currentuser,ACCESS_DENIED);
			return(-1);
		}

		strcpy(rooms[RoomCount].owner,owner);   

		DatabaseUpdated=TRUE;       			/* update database */
		return(0);
	}

}
		
SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(0);
}

/* copy object */

int CopyObject(user *currentuser,char *ObjectName,int DestinationRoom) {
roomobject *ObjectPtr;
user *usernext;
room *CurrentRoom;
int found=FALSE;
int count;
int CopyDestination;

CurrentRoom=currentuser->roomptr;

if(currentuser->status < WIZARD) {      /* not wizard */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(GetNumberOfRooms() > CurrentRoom->room) {			/* can't find room */
	SetLastError(currentuser,BAD_ROOM);  
	return(-1);
}

/* move object */
	
ObjectPtr=CurrentRoom->roomobjects;

while(ObjectPtr != NULL) {
	if(regexp(ObjectPtr->name,ObjectName) == TRUE) {				/* if object matches */

		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(CurrentRoom->owner,currentuser->name) == 0) && (CurrentRoom->attr & OBJECT_MOVEABLE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			if((strcmp(CurrentRoom->owner,currentuser->name) == 0) && (CurrentRoom->attr & OBJECT_MOVEABLE_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		/* copy object */
		if(rooms[DestinationRoom].roomobjects == NULL) {
			rooms[DestinationRoom].roomobjects=calloc(1,sizeof( roomobject));	/* allocate objects */ 
			if(rooms[DestinationRoom].roomobjects == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);  
				return(-1);
			}

			rooms[DestinationRoom].roomobjects_last=rooms[DestinationRoom].roomobjects;
		}
		else
		{
			rooms[DestinationRoom].roomobjects_last->next=calloc(1,sizeof( roomobject));	/* allocate objects */ 
			if(rooms[DestinationRoom].roomobjects == NULL) {		/* can't allocate */
				SetLastError(currentuser,NO_MEM);  
				return(-1);
			}

			rooms[DestinationRoom].roomobjects_last=rooms[DestinationRoom].roomobjects_last->next;
		}

		memcpy(rooms[DestinationRoom].roomobjects_last,ObjectPtr,sizeof(roomobject));		/* copy object */
		
		found=TRUE;
	}

	ObjectPtr=ObjectPtr->next;
}

/*
* move player
*/

usernext=GetUserPointerByName(ObjectName);		/* find user */

if(usernext != NULL) {			/* found user */
	go(usernext,ObjectName);
	found=TRUE;
}

/* copy room */

if((char) *ObjectName == '#') {
	sscanf(ObjectName,"#%d",&CopyDestination);

	memcpy(&rooms[CopyDestination],&rooms[DestinationRoom],sizeof(room));
	found=TRUE;
}

if(found == FALSE)  {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  
	return(-1);
}

return(0);
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
if(handle == NULL) return(-1);		/* can't open configution file */

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

for(RoomCount=0;RoomCount < GetNumberOfRooms(); RoomCount++) {

	if((rooms[RoomCount].attr & ROOM_HAVEN) == 0) {          /* not haven */
		NumberOfObjectsToGenerate=rand() % (config.roomobjectnumber + 1) - 0;

		while(NumberOfObjectsToGenerate > 0) {
			RandomObjectNumber=rand() % (TotalObjectCount + 1) - 0;

			TokenizeLine(&ObjectsList[RandomObjectNumber*BUF_SIZE],tokens,":");		/* tokenize line */
			
			if(IsObjectInRoom(tokens[OBJECT_NAME],RoomCount) == FALSE) { 		/* if object is not in room */
				if(rooms[RoomCount].roomobjects == NULL) {	/* no objects in room */
					rooms[RoomCount].roomobjects=calloc(1,sizeof(roomobject));		/* add link to end */
					if(rooms[RoomCount].roomobjects == NULL) continue;

					rooms[RoomCount].roomobjects_last=rooms[RoomCount].roomobjects;
				}
        			else
        			{
					rooms[RoomCount].roomobjects_last->next=calloc(1,sizeof(roomobject));		/* add link to end */
					rooms[RoomCount].roomobjects_last=rooms[RoomCount].roomobjects_last->next;
				}

				/* add object to room */

				rooms[RoomCount].roomobjects_last->room=RoomCount;
				strcpy(rooms[RoomCount].roomobjects_last->name,tokens[OBJECT_NAME]);
				strcpy(rooms[RoomCount].roomobjects_last->owner,"nobody");
				strcpy(rooms[RoomCount].roomobjects_last->desc,tokens[OBJECT_DESCRIPTION]);
				rooms[RoomCount].roomobjects_last->attackpoints=atoi(tokens[OBJECT_ATTACKPOINTS]);
				rooms[RoomCount].roomobjects_last->generateprob=atoi(tokens[OBJECT_GENERATEPROB]);
				rooms[RoomCount].roomobjects_last->staminapoints=atoi(tokens[OBJECT_STAMINAPOINTS]);
				rooms[RoomCount].roomobjects_last->magicpoints=atoi(tokens[OBJECT_MAGICPOINTS]);
				rooms[RoomCount].roomobjects_last->attr=OBJECT_TEMPORARY;

				NumberOfObjectsToGenerate--;
			}   

			
		}
	}

}

free(ObjectsList);
return(0);
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

	if(strcmp(CurrentRoom->owner,currentuser->name) == 0) {

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


/* create object */

int CreateObject(user *currentuser,char *ObjectName) {
roomobject *ObjectPtr;
roomobject *ObjPtrLast;
room *CurrentRoom;
char *buf[BUF_SIZE];

CurrentRoom=currentuser->roomptr;

if(currentuser->status < WIZARD) {         /* can't create object unless wizard level or above */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/*
* deny permission if not archwizard and owner doesn't allow others to create objects in room
*/

if(currentuser->status < ARCHWIZARD) {
	if((strcmp(CurrentRoom->owner,currentuser->name) !=0) && (CurrentRoom->attr  & ROOM_CREATE_PUBLIC) == 0 && currentuser->status < ARCHWIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

/*
	* deny permission if not archwizard and owner not allowed to create objects in room
	*/

	if((strcmp(CurrentRoom->owner,currentuser->name) == 0) && (CurrentRoom->attr  & ROOM_CREATE_OWNER) == 0 && currentuser->status < ARCHWIZARD) {
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}
}

/*
* check if object already exists and display error message if it does
*/

ObjectPtr=CurrentRoom->roomobjects;

while(ObjectPtr != NULL) {
	
	if(strcmp(ObjectName,ObjectPtr->name) == 0) {
		SetLastError(currentuser,OBJECT_EXISTS);  
		return(-1);
	}

	ObjPtrLast=ObjectPtr;
	ObjectPtr=ObjectPtr->next;
}
	
if(CurrentRoom->roomobjects == NULL) {		/* no objects */
	CurrentRoom->roomobjects=calloc(1,sizeof(roomobject));		/* add link to end */
	if(CurrentRoom->roomobjects == NULL) {		/* can't allocate */
		SetLastError(currentuser,NO_MEM);  
		return(-1);
	}

	ObjectPtr=CurrentRoom->roomobjects;
}
else
{
	ObjectPtr=CurrentRoom->roomobjects_last;
}


ObjectPtr->next=calloc(1,sizeof(roomobject));
if(ObjectPtr == NULL) {		/* can't allocate */
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
ObjectPtr->next=NULL;

DatabaseUpdated=TRUE;                                     /* mark database as updated */
return(0);
}

/* delete object */

int DeleteObject(user *currentuser,char *ObjectName) {
roomobject *ObjectPtr;
roomobject *ObjPtrLast;
room *CurrentRoom;

CurrentRoom=currentuser->roomptr;

if(currentuser->status < WIZARD) {             /* can't do this unless wizard of higher level */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/*
* find object
*/

ObjectPtr=CurrentRoom->roomobjects;
ObjPtrLast=ObjectPtr;

while(ObjectPtr != NULL) {
	ObjPtrLast=ObjectPtr;

	if(regexp(ObjectPtr->name,ObjectName) == TRUE) {		/* found */

		/*
		* can't delete if not owner and OBJECT_DELETE_PUBLIC attribute not set
		*/


		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(ObjectPtr->owner,currentuser->name) != 0) && (ObjectPtr->attr & OBJECT_DELETE_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			/*
			* if object owner and OBJECT_MOVEABLE_OWNER attribute not set, display error message
			*/

			if((strcmp(ObjectPtr->owner,currentuser->name) == 0) && (ObjectPtr->attr & OBJECT_DELETE_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		/*
		*
		* remove object from room
		*/


		if(ObjectPtr == CurrentRoom->roomobjects) {		/* first object */
			ObjectPtr=ObjectPtr->next;

			free(CurrentRoom->roomobjects);   
			CurrentRoom->roomobjects=ObjectPtr;
		}
					    
		if(ObjectPtr != CurrentRoom->roomobjects && ObjectPtr->next == NULL) {		/* last object */
			free(ObjectPtr);	
			
			CurrentRoom->roomobjects_last=ObjPtrLast->next;	/* new end of list */
		}

		if(ObjectPtr != CurrentRoom->roomobjects && ObjectPtr->next != NULL) {      
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


int RenameObject(user *currentuser,char *ObjectName,char *n) {
user *usernext;
roomobject *ObjectPtr;
room *CurrentRoom;

CurrentRoom=currentuser->roomptr;

if(currentuser->status < WIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

/* renaming user */

usernext=GetUserPointerByName(ObjectName);		/* find user */
if(usernext != NULL) {			/* found user */
	strcpy(usernext,n);	/* set username */

	if(UpdateUser(currentuser,currentuser->name,"",0,0,"",0,currentuser->staminapoints,0,0,"","",0) == -1) return(-1);
}



/* renaming object */
ObjectPtr=CurrentRoom->roomobjects;

	while(ObjectPtr->next != NULL) {

		if(regexp(ObjectPtr->name,ObjectName) == TRUE) {		/* found user */
			/* check permissions */

			if(currentuser->status < ARCHWIZARD) {

			/* is owner but can't rename */

				if(strcmp(ObjectPtr->owner,currentuser->next) == 0 && (ObjectPtr->attr & OBJECT_RENAME_OWNER) == 0) {
					SetLastError(currentuser,ACCESS_DENIED);
					return(-1);
				}

			/* not owner and can't rename */

			if(strcmp(ObjectPtr->owner,currentuser->next) != 0 && (ObjectPtr->attr & OBJECT_RENAME_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}

		strcpy(ObjectPtr->name,n);		/* rename object */
		return(0);
	}

	ObjectPtr=ObjectPtr->next;
}

SetLastError(currentuser,OBJECT_NOT_FOUND);  
return(-1);
}

/* look at object, user or room */

int look(user *currentuser,char *username) {
char *ErrorMessage[BUF_SIZE];
roomobject *ObjectNext;
user *usernext;
room *CurrentRoom;
int found=FALSE;
int RoomExitCount;
monster *RoomMonster;

CurrentRoom=currentuser->roomptr;
	
/* no name, so look at current room */

if(!*username) {				/* display name */
	sprintf(ErrorMessage,"\r\n#%d %s\r\n",CurrentRoom->room,CurrentRoom->name);

	send(currentuser->handle,ErrorMessage,strlen(ErrorMessage),0);  
	send(currentuser->handle,CurrentRoom->desc,strlen(CurrentRoom->desc),0);  

	send(currentuser->handle,"\r\nExits: ",8,0);  		/* display exits */

	for(RoomExitCount=0;RoomExitCount<11;RoomExitCount++) {
		if(CurrentRoom->exits[RoomExitCount] != 0) send(currentuser->handle,RoomExits[RoomExitCount],strlen(RoomExits[RoomExitCount]),0);
	}

	send(currentuser->handle,"\r\n",2,0);

	if(CurrentRoom->roomobjects != NULL) {		/* display objects */
		send(currentuser->handle,"\r\n",2,0);
		send(currentuser->handle,RoomMessage,strlen(RoomMessage),0);
		ObjectNext=CurrentRoom->roomobjects;

		while(ObjectNext != NULL) {
			send(currentuser->handle,ObjectNext->name,strlen(ObjectNext->name),0);
			send(currentuser->handle," ",1,0);

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
		sprintf(ErrorMessage,"a %s is here\r\n",RoomMonster->name);

		send(currentuser->handle,ErrorMessage,strlen(ErrorMessage),0);

		RoomMonster=FindNextMonsterInRoom(RoomMonster);
	} 


/*
* display users in room
*/

usernext=FindFirstUser();		/* find first user */

while(usernext != NULL) {
	if((regexp(usernext->name,username) == TRUE) && (usernext->loggedin == TRUE) && (usernext->room == currentuser->room)) {
		if(usernext->gender == MALE) {
			sprintf(ErrorMessage,"%s the %s is here\r\n",usernext->name,GetPointerToMaleTitles(usernext->status));
		}
		else
		{
			sprintf(ErrorMessage,"%s the %s is here\r\n",usernext->name,GetPointerToFemaleTitles(usernext->status));
		}

		send(currentuser->handle,ErrorMessage,strlen(ErrorMessage),0);
	}

	usernext=FindNextUser(usernext);		/* find next user */
}

return(0);
}


/*
* looking at object or person
*/

ObjectNext=CurrentRoom->roomobjects;

while(NULL != ObjectNext) {
	if(ObjectNext == NULL) break;

	if(regexp(ObjectNext->name,username) == TRUE) {
		send(currentuser->handle,ObjectNext->desc,strlen(ObjectNext->desc),0); /* if object matches */
		found=TRUE;
	}

	ObjectNext=ObjectNext->next;
}
	

/*
* if not not object or room, search for user
*/

usernext=FindFirstUser();		/* find first user */

while(usernext != NULL) {
	if((regexp(usernext->name,username) == TRUE) && (usernext->loggedin == TRUE) && (usernext->room == currentuser->room)) {
		sprintf(ErrorMessage,"%s\r\n",usernext->desc);		/* show description */
		send(currentuser->handle,ErrorMessage,strlen(ErrorMessage),0);

		found=TRUE;

	/* if the user is a wizard tell them they have looked at them */

		sprintf(ErrorMessage,"%s has looked at you\r\n",currentuser->name);
		if(usernext->status >= WIZARD) send(usernext->handle,ErrorMessage,strlen(ErrorMessage),0);
	}

	usernext=FindNextUser(usernext);		/* find next user */
}

/*
* if monster
*
*/

RoomMonster=FindFirstMonsterInRoom(currentuser->room);

while(RoomMonster != NULL) {
	if(regexp(username,RoomMonster->name) == TRUE) {
		send(currentuser->handle,RoomMonster->desc,strlen(RoomMonster->desc),0);

		found=TRUE;
		return(0);
	}

	RoomMonster=FindNextMonsterInRoom(RoomMonster);
};


/* can't find it, so output error message and exit */

if(found == FALSE) {
	SetLastError(currentuser,OBJECT_NOT_FOUND);  
	return(-1);
}

return(0);
}



/* copy file */

int CopyFile(char *source,char *destination) {
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

int describe(user *currentuser,char *ObjectName,char *d) {
user *usernext;
roomobject *roomobject;
room *CurrentRoom;
int count;
CONFIG config;

GetConfigurationInformation(&config);

CurrentRoom=currentuser->roomptr;

if(strcmp(ObjectName,"me") == 0) {                           /* if setting description for self */
	return(UpdateUser(currentuser,currentuser->name,"",0,0,d,0,currentuser->staminapoints,0,0,"","",0));
}

/*
* set description for other user
*
*/

usernext=FindFirstUser();		/* find first user */

while(usernext != NULL) {
	if((regexp(usernext->name,ObjectName) == TRUE) && (usernext->loggedin == TRUE) && (usernext->room == currentuser->room)) {
		if(currentuser->status < WIZARD) {  /* can't change other user's description unless you are wizard or higher */
			SetLastError(currentuser,NOT_YET);
			return(-1);
		}

		return(UpdateUser(currentuser,ObjectName,"",0,0,d,0,usernext->staminapoints,0,0,"","",0));
	}

	usernext=FindNextUser(usernext);		/* find next user */
}

/*
* set object description
*/
roomobject=CurrentRoom->roomobjects;

while(roomobject != NULL) {
	if(regexp(roomobject->name,ObjectName) == TRUE) {		/* found user */
		/* if owner and OBJECT_RENAME_OWNER attribute not set, then display an error message */

		if(currentuser->status < ARCHWIZARD) {
			if((strcmp(rooms[count].owner,currentuser->name) == 0) && (roomobject->attr & OBJECT_RENAME_OWNER) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}

			/* if not owner and OBJECT_RENAME_PUBLIC attribute not set, then display an error message */

			if((strcmp(rooms[count].owner,currentuser->name) != 0) && (roomobject->attr & OBJECT_RENAME_PUBLIC) == 0) {
				SetLastError(currentuser,ACCESS_DENIED);
				return(-1);
			}
		}


		strcpy(roomobject->desc,d);               /* set object description	*/
		DatabaseUpdated=TRUE;                     /* update database */
	}

	roomobject=roomobject->next;
}

	
if(strcmp(ObjectName,"here") == 0) {                           /* if setting description for room */
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

	strcpy(CurrentRoom->desc,d);
	DatabaseUpdated=TRUE;

	return(0);
}

/*
* set room description
*/

for(count=0;count<GetNumberOfRooms();count++) {
	if(regexp(rooms[count].name,ObjectName) == TRUE) {                    /* found object */
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
		DatabaseUpdated=TRUE;                               /* update database */
		return(0);
	}

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

int GetRoomFlags(int RoomNumber) {
return(rooms[RoomNumber].attr);
}

char *GetRoomName(int RoomNumber) {
return(&rooms[RoomNumber].name);
}

room *GetRoomPointer(int RoomNumber) {
return(&rooms[RoomNumber]);
}

int GetRoomMonsterEvil(int RoomNumber,int RoomMonster) {
return(rooms[RoomNumber].roommonsters[RoomMonster].evil);
}

char *GetRoomMonsterName(int RoomNumber,int RoomMonster) {
return(&rooms[RoomNumber].roommonsters[RoomMonster].name);
}

int GetNumberOfMonstersInRoom(int RoomNumber) {
return(rooms[RoomNumber].monstercount);
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

