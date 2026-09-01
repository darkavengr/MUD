#include "size.h"

#ifndef CONFIG_H
	#define CONFIG_H

	typedef struct {
		char *server[BUF_SIZE];
		int port;
		int ObjectGenerateTime;
		int ConfigurationSaveTime;
		int BackupDatabase;
		int AllowPlayerKilling;
		int AllowNewAccounts;
		int MonsterGenerateTime;
		int PointsForWarrior;
		int PointsForHero;
		int PointsForChampion;
		int PointsForSuperhero;
		int PointsForEnchanter;
		int PointsForSorceror;
		int PointsForNecromancer;
		int PointsForLegend;
		int PointsForWizard;
		char *BannerMessage;
		int roomobjectnumber;
	} CONFIG;
#endif

