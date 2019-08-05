#define NULL 0
#define TRUE	1
#define FALSE 	0

#define DEFAULT_STAMINAPOINTS  1000                               /* default number of stamina points */
#define DEFAULT_MAGICPOINTS  1000                               /* default number of magic points */
#define MONSTERGEN	10

#define MUD_NAME  "AdventureMUD"

#define MAJOR_VERSION	2
#define MINOR_VERSION	0

#define USER_INVISIBLE 1
#define USER_GAGGED  2

#define HELPPAGESIZE	30
#define MAXROOMOBJECTS	10
#define LISTEN_BACKLOG 50

#define NOCONFIGFILE  1
#define BADCONFIG     2
#define NOMEM 	      3
#define NO_SOCKET     4
#define NO_BIND	      5
#define NO_LISTEN     6
#define NO_THREAD     7
#define	NO_RESOLVE    8
#define CONFIG_ERROR  10

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

#define BUF_SIZE 255	
#define PLAYERVERSUSMONSTER  2                                        /* player versus monster */
#define PLAYERVERSUSPLAYER   3                                        /* player versus player */
                        
#define OBJECT_NAME 		0                                                  /* array entry for object info */
#define OBJECT_STAMINAPOINTS	1
#define OBJECT_MAGICPOINTS 	2
#define OBJECT_ATTACKPOINTS	3
#define OBJECT_GENERATEPROB	4
#define OBJECT_DESCRIPTION	5
#define OBJECT_OWNER		6
#define OBJECT_ATTR		7

#define MALE 			2                                                       /* genders */
#define FEMALE 			4

#define OBJECT_DELETE_OWNER	1                                          /* object properties */
#define OBJECT_DELETE_PUBLIC	2
#define OBJECT_MOVEABLE_OWNER	4
#define OBJECT_MOVEABLE_PUBLIC	8
#define OBJECT_PICKUP_OWNER	16
#define OBJECT_PICKUP_PUBLIC	32
#define OBJECT_RENAME_OWNER	64
#define OBJECT_RENAME_PUBLIC	128
#define OBJECT_TEMPORARY	256

#define ROOM_CREATE_OWNER	1                                            /* room properties */
#define ROOM_CREATE_PUBLIC	2
#define ROOM_EXIT_OWNER		4
#define ROOM_EXIT_PUBLIC	8
#define ROOM_RENAME_OWNER	16
#define ROOM_RENAME_PUBLIC	32
#define ROOM_HAVEN		64
#define ROOM_PRIVATE		128
#define ROOM_DEAD		256

#define	STATE_GETUSER		0
#define	STATE_GETPASSWORD	1
#define	STATE_CHECKLOGIN	2
#define	STATE_GETNEWPASS	3
#define	STATE_GETGENDER		4
#define	STATE_GETDESC		5
#define	STATE_GETRACE		6
#define	STATE_GETCLASS		7
#define	STATE_CREATEUSER	8
#define STATE_GETCOMMAND	255

#define MONSTER_SPAWN_PROB	5
#define ROOM_MONSTER_COUNT	10

#define NORTH			0
#define SOUTH			1
#define EAST			2
#define WEST			3
#define NORTHEAST		4
#define NORTHWEST		5
#define SOUTHEAST		6
#define SOUTHWEST		7
#define	UP			8
#define DOWN			9


#define MONSTER_SPAWN_PROB	5
#define ROOM_MONSTER_COUNT	10

typedef struct {			/* monsters */
 char *name[BUF_SIZE];
 char *desc[BUF_SIZE];
 struct room *roomptr;
 int stamina;
 int evil;
 char *arrivemessage[BUF_SIZE];
 char *leavemessage[BUF_SIZE];
 char *createmessage[BUF_SIZE];
 char *diemessage[BUF_SIZE];
 int moveodds;
 int genodds;
 int damage;
 int room;
 int sleep;
 struct monster *last;
 struct monster *next;
} monster;

typedef struct {			/* classes */
 char *name[BUF_SIZE];
 struct race *next;
} class;

typedef struct {				/* race */
 char *name[BUF_SIZE];
 int magic;
 int strength;
 int agility;
 int dexterity;
 int luck;
 int wisdom;
 int intelligence;
 int stamina;
 struct race *next;
} race;


typedef struct {			/* objects */
 int room;
 char *owner[BUF_SIZE];
 char *name[BUF_SIZE];
 char *desc[BUF_SIZE];
 int attackpoints;
 int generateprob;
 int staminapoints;
 int magicpoints;
 int attr;
 struct roomobject *next;
} roomobject;

typedef struct {				/* rooms */
 int room;
 char *name[BUF_SIZE];
 char *owner[BUF_SIZE];
 char *desc[BUF_SIZE];
 int attr;
 int exits[11];
 struct mudobject *roomobjects;
 int monstercount;
 monster roommonsters[ROOM_MONSTER_COUNT];
} room;

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
 char *ip[BUF_SIZE];
 struct mudobject *carryobjects;
 struct user *last;
 struct room *roomptr;
 struct user *next;
 char *roomname[BUF_SIZE];
 char *ipaddress[BUF_SIZE];
} user;

typedef struct {
 char *name[BUF_SIZE];
 int spellpoints;
 int damage;
 char *message[BUF_SIZE];
 int level;
 char *class[BUF_SIZE];
 struct spell *next;
} spell;

#ifdef _WIN32
 typedef int socklen_t;
#endif

typedef struct {
 char *ipaddress[BUF_SIZE];
 struct ban *next;
} ban;


