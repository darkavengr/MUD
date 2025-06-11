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

#define DEFAULT_STAMINAPOINTS  1000                               /* default number of stamina points */
#define DEFAULT_MAGICPOINTS  1000                               /* default number of magic points */

#define MALE 			0                                                       /* genders */
#define FEMALE 			1

#ifndef USER_H
#define USER_H
typedef struct user {
 char *name[BUF_SIZE];
 char *password[BUF_SIZE];
 char *desc[BUF_SIZE];
 int status;
 int gender;
 unsigned int room;
 unsigned int homeroom;
 int magicpoints;
 int staminapoints;
 int experiencepoints;
 int loggedin;
 struct race *race;
 struct class *userclass;
 int handle;
 int flags;
 int lasterror;
 char *ip[BUF_SIZE];
 struct roomobject *carryobjects;
 struct roomobject *carryobjects_last;
 struct user *last;
 struct room *roomptr;
 struct user *next;
 char *roomname[BUF_SIZE];
 char *ipaddress[BUF_SIZE];
} user;
#endif

int BanUserByName(user *currentuser,char *username);
int BanUserByIPAddress(user *currentuser,char *ipaddr);
int UpdateBanFile(void);
int ListBans(user *currentuser,char *banname);
int UnBanUserByIPAddress(user *currentuser,char *ipaddr);
int LoadBans(void);
int CheckIfBanned(char *name);
int ForceUser(user *currentuser,char *u,char *c);
int GiveObjectToUser(user *currentuser,char *u,char *o);
int DisplayInventory(user *currentuser,char *u);
int KillUser(user *currentuser,char *u);
int pose(user *currentuser,char *msg);
void quit(user *currentuser);
int DisplayScore(user *currentuser,char *u);
int SendMessageToAllInRoom(int room,char *msg);
int SendMessage(user *currentuser,char *nick,char *msg);
int TakeObject(user *currentuser,char *username,char *object);
int UpdateUser(user *currentuser,char *uname,char *upass,int uhome,int ulevel,char *udesc,int umpoints,int ustapoints,int uexpoints,int ugender,char *racex,char *classx,int uflags);
int UpdateUsersFile(void);
int SetPoints(user *currentuser,char *u,char *amountstr,int which);
int SetUserLevel(user *currentuser,char *u,char *level);
int SetUserGender(user *currentuser,char *u,char *gender);
int LoadRaces(void);
int LoadClasses(void);
int LoadUsers(void);
int SetVisibleMode(user *currentuser,char *name,int mode);
int GagUser(user *currentuser,char *name,int mode);
int SendMessageToAll(user *currentuser,char *m);
int who(user *currentuser,char *username);
int go(user *currentuser,int roomnumber);
int SetInvisibleStatus(user *currentuser,char *username,int which);
int MoveObject(user *currentuser,char *objectname,int roomnumber);
int GetUser(char *name,user *buf);
int LoginUser(int msgsocket,char *uname,char *upass);
int CreateUser(int socket,char *name,char *pass,int gender,char *description,char *racex,char *classx);
int AddNewrace(race *newrace);
int AddNewClass(user *currentuser,class *newclass);
user *GetUserPointerByName(char *name);
user *FindFirstUser(void);
user *FindNextUser(user *last);
race *FindFirstRace(void);
race *FindNextRace(race *last);
char *GetPointerToMaleTitles(int level);
char *GetPointerToFemaleTitles(int level);
void AttackUser(int roomnumber,int roommonster);
void DisconnectAllUsers(void);
int UpdateUsersFile(void);

