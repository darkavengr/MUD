#include <stdbool.h>
#include "config.h"

int GetConfiguration(void);
int UpdateConfiguration(void);
void GetConfigurationInformation(CONFIG *buf);
void UpdateConfiguratonInformationFile(CONFIG *buf);
sqlite3 *GetDatabaseHandle(void);
void CloseDatabase(void);
char *GetDatabaseFilename(void);
bool GetConfigurationUpdatedFlag(void);
void SetConfigurationUpdatedFlag(bool WasUpdated);


