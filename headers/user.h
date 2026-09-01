#include <sqlite3.h>
#include "room.h"
#include "class.h"
#include "race.h"

#define USERNAME      0                                                   /* array entry for user info */
#define PASSWORD      1
#define HOMEROOM      2
#define USERLEVEL     3
#define DESCRIPTION   4
#define MAGICPOINTS   5
#define STAMINAPOINTS  6
#define EXPERIENCEPOINTS  7
#define GENDER 	     8
#define RACE 	     9
#define CLASS        10
#define USERFLAGS    11

#define NOVICE      1                                                     /* user levels */
#define WARRIOR     2
#define HERO        3
#define CHAMPION    4
#define SUPERHERO   5
#define ENCHANTER   6
#define SORCEROR    7
#define NECROMANCER 8
#define LEGEND      9
#define WIZARD      10
#define ARCHWIZARD  11
#define DUNGEONMASTER  12

#define USER_INVISIBLE 1
#define USER_GAGGED  2

#define DEFAULT_STAMINA_POINTS  1000                               /* default number of stamina points */
#define DEFAULT_MAGIC_POINTS  	100                               /* default number of magic points */
#define DEFAULT_USER_LEVEL	1
#define DEFAULT_EXPERIENCE_POINTS	0
#define DEFAULT_HOME_ROOM	1

#define MALE 		0                                                       /* genders */
#define FEMALE 		1

#define BAN_IPADDRESS_COLUMN	0
#define BAN_REASON_COLUMN	1

#define INVENTORY_ID_COLUMN	1
#define INVENTORY_NAME_COLUMN	2
#define INVENTORY_ATTR_COLUMN	3
#define INVENTORY_DESC_COLUMN	4
#define INVENTORY_OWNER_COLUMN	1

#ifndef USER_H
	#define USER_H

	typedef struct user {
		char *username[BUF_SIZE];
		char *password[BUF_SIZE];
		char *description[BUF_SIZE];
		int userlevel;
		int gender;
		unsigned int room;
		unsigned int homeroom;
		int magicpoints;
		int staminapoints;
		int experiencepoints;
		race race;
		userclass userclass;
		int socket;
		int flags;
		int lasterror;
		char *ip[BUF_SIZE];
		struct roomobject *carryobjects;
		struct roomobject *carryobjects_last;
		struct user *last;
		struct room *roomptr;
		char *roomname[BUF_SIZE];
		char *ipaddress[BUF_SIZE];
		struct user *prev;
		struct user *next;
	} user;
#endif

int ListBans(user *currentuser,char *ipaddress);
int BanUserByIPAddress(user *currentuser,char *ipaddress,char *reason);
int UnBanUserByIPAddress(user *currentuser,char *ipaddress);
int ForceUser(user *currentuser,char *username,char *command);
int GiveObjectToUser(user *currentuser,char *objectname,char *username);
int KillUser(user *currentuser,char *username);
int pose(user *currentuser,char *message);
void quit(user *currentuser);
int SendMessageToAllInRoom(int room,char *message);
int SendMessage(user *currentuser,char *username,char *message);
int TakeObject(user *currentuser,char *username,char *object);
int UpdateUser(user *currentuser,char *username,char *newusername,char *password,int homeroom,int userlevel,char *description,int magicpoints,int staminapoints,int experiencepoints,int gender,char *racex,char *classx,int flags);
int SetUserPoints(user *currentuser,char *username,char *amountstr,int which);
int SetUserLevel(user *currentuser,char *username,char *levelstr);
int SetUserGender(user *currentuser,char *username,char *gender);
int SetVisibleMode(user *currentuser,char *name,int mode);
int GagUser(user *currentuser,char *name,int mode);
int wall(user *currentuser,char *message);
int go(user *currentuser,int RoomNumber);
int MoveObject(user *currentuser,char *ObjectName,int RoomNumber);
int LoginUser(int messagesocket,char *username,char *password);
int CreateUser(int socket,char *username,char *password,char *description,int userlevel,int gender,int homeroom,int magicpoints,int staminapoints,int experiencepoints,char *racex,char *classx,int flags);
int AddNewRace(user *currentuser,race *newrace);
int AddNewClass(user *currentuser,userclass *newclass);
user *GetUserPointerByName(char *name);
user *FindFirstUser(void);
user *FindNextUser(user *last);
char *GetPointerToMaleTitles(int level);
char *GetPointerToFemaleTitles(int level);
void AttackUser(int RoomNumber,int roommonster);
int PickUpObject(user *currentuser,char *ObjectName);
int DropObject(user *currentuser,char *ObjectName);
int GetRace(char *name,race *out);
int GetClass(char *name,userclass *out);
int SetInventoryObjectOwnerInDatabase(char *owner,char *name);
int CheckIfBanned(char *ipaddress);
int RemoveFromInventoryInDatabase(user *currentuser,char *ObjectName,char *owner);
int CheckIfUserExists(char *username);
int CheckIfRaceExists(char *name);
int CheckIfClassExists(char *name);
int DisconnectUser(user *currentuser,char *username);

