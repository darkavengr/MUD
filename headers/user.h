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
typedef struct {
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

int userban(user *currentuser,char *username);
int banip(user *currentuser,char *ipaddr);
int updatebanfile(void);
int listbans(user *currentuser,char *banname);
int unbanip(user *currentuser,char *ipaddr);
int loadbans(void);
int checkban(char *name);
int force(user *currentuser,char *u,char *c);
int give(user *currentuser,char *u,char *o);
int inventory(user *currentuser,char *u);
int killuser(user *currentuser,char *u);
int pose(user *currentuser,char *msg);
void quit(user *currentuser);
int score(user *currentuser,char *u);
int sendmudmessagetoall(int room,char *msg);
int sendmudmessage(user *currentuser,char *nick,char *msg);
int take(user *currentuser,char *u,char *o);
int updateuser(user *currentuser,char *uname,char *upass,int uhome,int ulevel,char *udesc,int umpoints,int ustapoints,int uexpoints,int ugender,char *racex,char *classx,int uflags);
int updateusersfile(void);
int setpoints(user *currentuser,char *u,char *amountstr,int which);
int setlevel(user *currentuser,char *u,char *level);
int setgender(user *currentuser,char *u,char *gender);
int loadraces(void);
int loadclasses(void);
int loadusers(void);
int visible(user *currentuser,char *name,int mode);
int gag(user *currentuser,char *name,int mode);
int wall(user *currentuser,char *m);
int who(user *currentuser,char *username);
int go(user *currentuser,int r);
int invisible(user *currentuser,char *u,int which);
int moveobject(user *currentuser,char *o,int l);
int getuser(char *name,user *buf);
int login(int msgsocket,char *uname,char *upass);
int createuser(int socket,char *name,char *pass,int gender,char *description,char *racex,char *classx);
int addnewrace(race *newrace);
int addnewclass(user *currentuser,class *newclass);
user *GetUserPointerByName(char *name);
user *FindFirstUser(void);
user *FindNextUser(user *last);
char *GetPointerToMaleTitles(int level);
char *GetPointerToFemaleTitles(int level);

