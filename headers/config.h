#include "size.h"

typedef struct {
	char *mudserver[BUF_SIZE];
	int mudport;
	int objectresettime;
	int databaseresettime;
	int banresettime;
	int userresettime;
	int configsavetime;
	int databasebackup;
	int allowplayerkilling;
	int allownewaccounts;
	int monsterresettime;
	int pointsforwarrior;
	int pointsforhero;
	int pointsforchampion;
	int pointsforsuperhero;
	int pointsforenchanter;
	int pointsforsorceror;
	int pointsfornecromancer;
	int pointsforlegend;
	int pointsforwizard;
	char *isbuf;
	int issuecount;
	int roomobjectnumber;
} CONFIG;
