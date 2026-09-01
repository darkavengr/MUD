#define NO_ERROR 		0	
#define NO_MEM			1
#define OBJECT_NOT_FOUND	2
#define USERNAME_REQURED 	3
#define USERNAME_EXISTS 	4
#define INVALID_GENDER		5
#define INVALID_CLASS		6
#define INVALID_RACE		7	
#define NO_PASSWORD		8
#define USER_BANNED		9
#define INVALID_LOGIN		10
#define PVP_NOT_ALLOWED		11
#define INVALID_COMMAND		12
#define NO_PARAMS 		13
#define INVALID_EXIT		14
#define CANT_CREATE_OBJECTS_HERE 15
#define OBJECT_EXISTS		16
#define ROOM_NOT_FOUND		17
#define CANT_CREATE_ROOM 	18
#define ACCESS_DENIED		19
#define WEAK_PASSWORD		20
#define SPELL_LEVEL_USER 	21
#define SPELL_NO_TARGET		22
#define SPELL_HAVEN		23
#define INSUFFICIENT_MAGIC_POINTS 24
#define SPELL_NOT_FOUND		25
#define ALREADY_BANNED		26
#define UNKNOWN_USER		27
#define INVALID_LEVEL		28
#define INVALID_DIRECTION	29
#define CANT_BE_KILLED		30
#define GAME_OVER		31
#define ATTACK_HAVEN		32
#define INVALID_HELP_TOPIC	33
#define KILL_WIZARD		34
#define ALREADY_HAVE_OBJECT	35
#define RACE_EXISTS		36
#define CLASS_EXISTS		37
#define SYNTAX_ERROR		38
#define IO_ERROR		39
#define VERB_NOT_FOUND		40
#define FILE_NOT_FOUND		41

#define NOCONFIGFILE  1
#define INVALID_CONFIG     2
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

