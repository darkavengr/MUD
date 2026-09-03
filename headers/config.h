#include <stdbool.h>
#include "size.h"

#ifndef CONFIG_H
	#define CONFIG_H

	typedef struct {
		int port;
		int ObjectGenerateTime;
		int ConfigurationSaveTime;
		int BackupDatabase;
		bool AllowPlayerKilling;
		bool AllowNewAccounts;
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
		int MaximumNumberOfObjectsPerRoom;
		int MaximumNumberOfLoginAttempts;
	} CONFIG;
#endif

