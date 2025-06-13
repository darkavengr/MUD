#define NO_ERROR 		0	
#define NO_MEM			1
#define OBJECT_NOT_FOUND	2
#define NOT_YET			3
#define USERNAME_REQURED 	4
#define USERNAME_EXISTS 	5
#define BAD_GENDER		6
#define BAD_CLASS		7
#define BAD_RACE		8	
#define NO_PASSWORD		9
#define USER_BANNED		10
#define INVALID_LOGIN		11
#define PVP_NOT_ALLOWED		12
#define BAD_COMMAND		13
#define NO_PARAMS 		14
#define INVALID_EXIT		15
#define CANT_CREATE_OBJECTS_HERE 16
#define OBJECT_EXISTS		17
#define ROOM_NOT_FOUND		18
#define CANT_CREATE_ROOM 	19
#define PERMISSION_DENIED	20
#define WEAK_PASSWORD		21
#define SPELL_LEVEL_USER 	22
#define SPELL_NO_TARGET		23
#define SPELL_HAVEN		24
#define INSUFFICIENT_MAGIC_POINTS 25
#define SPELL_NOT_FOUND		26
#define ALREADY_BANNED		27
#define UNKNOWN_USER		28
#define INVALID_LEVEL		29
#define BAD_DIRECTION		30
#define CANT_BE_KILLED		31
#define GAME_OVER		32
#define ATTACK_HAVEN		33
#define INVALID_HELP_TOPIC	34
#define KILL_WIZARD		35
#define NO_PASSWORD		36
#define ACCESS_DENIED		37
#define BAD_ROOM		38
#define ALREADY_HAVE_OBJECT	39
#define RACE_EXISTS		40
#define CLASS_EXISTS		41
#define SYNTAX_ERROR		42
#define IO_ERROR		34

#define NOCONFIGFILE  1
#define BADCONFIG     2
#define NOMEM 	      3
#define NO_SOCKET     4
#define NO_BIND	      5
#define NO_LISTEN     6
#define NO_THREAD     7
#define	NO_RESOLVE    8
#define CONFIG_ERROR  10

#include "user.h"

void SetLastError(user *user,int error);
int GetLastError(user *user);
void PrintError(int socket,int error);

