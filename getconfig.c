/*
	Adventure MUD server 

	(c) Copyright Matthew Boote 2018, All rights reserved blah blah blah etc etc etc 

		This program is free software: you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation, either version 3 of the License, or
		(at your option) any later version.

		This program is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		GNU General Public License for more details.

		You should have received a copy of the GNU General Public License
		along with this program.  If not, see <http:www.gnu.org/licenses/>.

*/

/* load configuration */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>
#include <stdbool.h>
#include "errors.h"
#include "config.h"
#include "bool.h"

CONFIG config;
char *BannerFile="config/banner.conf";
char *ConfigurationFile="config/mud.conf";
char *DatabaseFilename="config/mud.db";
char *MustBeTrueOrFalse="mud: %d: value must be true or false\n";
sqlite3 *DatabaseHandle=NULL;
bool ConfigurationUpdated=FALSE;

int GetConfiguration(void) {
FILE *handle;
char *LineBuffer[BUF_SIZE];
int LineCount;
char *LineTokens[BUF_SIZE][BUF_SIZE];
int ErrorCount=0;
int BannerMessageSize;

ConfigurationUpdated=FALSE;

/* load general configuration */

printf("Loading configuration...");

handle=fopen(ConfigurationFile,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("mud: Can't open configuration file %s\n",ConfigurationFile);
	exit(NOCONFIGFILE);
}

LineCount=0;

while(!feof(handle)) {
	fgets(LineBuffer,BUF_SIZE,handle);			/* get data */

	if((char) *LineBuffer == '\n') continue;		/* skip blank lines */
	if((char) *LineBuffer == '#') continue;			/* skip comments */

	RemoveNewLine(LineBuffer);				/* remove newline character */

	TokenizeLine(LineBuffer,LineTokens,"=");			/* tokenize line */

	LineCount++;                                          /* line count */

	if(strncmp(LineTokens[0],"port",BUF_SIZE) == 0) {
		config.port=atoi(LineTokens[1]);
	}
	else if(strncmp(LineTokens[0],"ObjectGenerateTime",BUF_SIZE) == 0) {        /* how often to generate objects */
		config.ObjectGenerateTime=GetValueFromTimeString(LineTokens[1]);
	}
	else if(strncmp(LineTokens[0],"ConfigurationSaveTime",BUF_SIZE) == 0) {		/* How often to save configuration */
		config.ConfigurationSaveTime=GetValueFromTimeString(LineTokens[1]);
	}
	else if(strncmp(LineTokens[0],"MaximumNumberOfObjectsPerRoom",BUF_SIZE) == 0) {        /* maximum number of objects per room */
		config.MaximumNumberOfObjectsPerRoom=atoi(LineTokens[1]);
	}
	else if(strncmp(LineTokens[0],"BackupDatabase",BUF_SIZE) == 0) {		/* backup database before save */
		config.BackupDatabase=-1;

		if(strncmp(LineTokens[1],"true",BUF_SIZE) == 0) config.BackupDatabase=TRUE;
		if(strncmp(LineTokens[1],"false",BUF_SIZE) == 0) config.BackupDatabase=FALSE;

		if(config.BackupDatabase == -1) printf(MustBeTrueOrFalse,LineCount);	/* invalid option */

	}
	else if(strncmp(LineTokens[0],"AllowPlayerKilling",BUF_SIZE) == 0) {		/* allow player killing */
		config.AllowPlayerKilling=-1;

		if(strncmp(LineTokens[1],"true",BUF_SIZE) == 0) config.AllowPlayerKilling=TRUE;
		if(strncmp(LineTokens[1],"false",BUF_SIZE) == 0) config.AllowPlayerKilling=FALSE;

		if(config.AllowPlayerKilling == -1) printf(MustBeTrueOrFalse,LineCount);	/* invalid option */
	 }
	 else if(strncmp(LineTokens[0],"AllowNewAccounts",BUF_SIZE) == 0) {		/* allow new accounts */
		config.AllowNewAccounts=-1;

		if(strncmp(LineTokens[1],"true",BUF_SIZE) == 0) config.AllowNewAccounts=TRUE;
		if(strncmp(LineTokens[1],"false",BUF_SIZE) == 0) config.AllowNewAccounts=FALSE;

		if(config.AllowNewAccounts == -1) printf(MustBeTrueOrFalse,LineCount);	/* invalid option */
	 }
	 else if(strncmp(LineTokens[0],"MonsterGenerateTime",BUF_SIZE) == 0) {		/* how often to reset monsters */
		config.MonsterGenerateTime=GetValueFromTimeString(LineTokens[1]);
	 }
	else if(strncmp(LineTokens[0],"MaximumNumberOfLoginAttempts",BUF_SIZE) == 0) {		/* maximum number of login attempts */
		config.MaximumNumberOfLoginAttempts=atoi(LineTokens[1]);
	 }
	 else if(strncmp(LineTokens[0],"PointsForWarrior",BUF_SIZE) == 0) {
		config.PointsForWarrior=atoi(LineTokens[1]);	/* points for levels */
	 }
	 else if(strncmp(LineTokens[0],"PointsForHero",BUF_SIZE) == 0) {
		config.PointsForHero=atoi(LineTokens[1]);
	 }
	 else if(strncmp(LineTokens[0],"PointsForChampion",BUF_SIZE) == 0) {
		config.PointsForChampion=atoi(LineTokens[1]);
	 }
	 else if(strncmp(LineTokens[0],"PointsForSuperhero",BUF_SIZE) == 0) {
		config.PointsForSuperhero=atoi(LineTokens[1]);
	 }
	 else if(strncmp(LineTokens[0],"PointsForEnchanter",BUF_SIZE) == 0) {
		config.PointsForEnchanter=atoi(LineTokens[1]);
	 }
	 else if(strncmp(LineTokens[0],"PointsForSorceror",BUF_SIZE) == 0) {
		config.PointsForSorceror=atoi(LineTokens[1]);
	 }
	 else if(strncmp(LineTokens[0],"PointsForNecromancer",BUF_SIZE) == 0) {
		config.PointsForNecromancer=atoi(LineTokens[1]);
	 }
	 else if(strncmp(LineTokens[0],"PointsForLegend",BUF_SIZE) == 0) {
		config.PointsForLegend=atoi(LineTokens[1]);
	 }
	 else if(strncmp(LineTokens[0],"PointsForWizard",BUF_SIZE) == 0) {
		config.PointsForWizard=atoi(LineTokens[1]);
	 }
	 else {
		 printf("\nmud: %d: unknown configuration option %s in %s\n",LineCount,LineTokens[0],ConfigurationFile);		/* unknown configuration option */
		 ErrorCount++;	
	}
}

fclose(handle);

if(ErrorCount == 0) {
	printf("ok\n");
}
else
{
	printf("\n");
}

/* load MUD database */

printf("Opening database...");

if(sqlite3_open(DatabaseFilename,&DatabaseHandle) == SQLITE_OK) {
	printf("ok\n");
}
else
{
	ErrorCount++;
}

printf("Loading world data...");

if(LoadWorld() == 0) {
	printf("ok\n");
}
else
{
	ErrorCount++;
}

printf("Loading world object data...");

if(LoadWorldObjects() == 0) {
	printf("ok\n");
}
else
{
	ErrorCount++;
}

printf("Loading banner message...");

/* LOAD issue message */
handle=fopen(BannerFile,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",BannerFile);
	exit(NOCONFIGFILE);
}

fseek(handle,0,SEEK_END);		/* get file size */
BannerMessageSize=ftell(handle);
fseek(handle,0,SEEK_SET);

config.BannerMessage=calloc(1,BannerMessageSize);
if(config.BannerMessage == NULL) {			/* can't allocate */
	perror("mud:");
	exit(NOMEM);
}

fread(config.BannerMessage,1,BannerMessageSize,handle);		/* read data */

fclose(handle);

printf("ok\n");

if(ErrorCount > 0) {			/* errors */
	printf("%d errors\n",ErrorCount);
	exit(CONFIG_ERROR);
}

return(0);
}

int UpdateConfigurationFile(void) {
FILE *handle;
char *buf[BUF_SIZE];

handle=fopen(ConfigurationFile,"w");
if(handle == NULL) return(-1);                                        /* couldn't open file */

fprintf(handle,"port=%d\n",config.port);

CreateTimeString(config.ObjectGenerateTime,buf);
fprintf(handle,"ObjectGenerateTime=%s\n",buf);

CreateTimeString(config.ConfigurationSaveTime,buf);
fprintf(handle,"ConfigurationSaveTime=%s\n",buf);

fprintf(handle,"MaximumNumberOfObjectsPerRoom=%d\n",config.MaximumNumberOfObjectsPerRoom);

fputs("BackupDatabase=",handle);
if(config.BackupDatabase == TRUE) {
	fputs("true\n",handle);
}
else
{
	fputs("false\n",handle);
}

fputs("AllowPlayerKilling=",handle);
if(config.AllowPlayerKilling == TRUE) {
	fputs("true\n",handle);
}
else
{
	fputs("false\n",handle);
}

fputs("AllowNewAccounts=",handle);
if(config.AllowNewAccounts== TRUE) {
	fputs("true\n",handle);
}
else
{
	fputs("false\n",handle);
}

CreateTimeString(config.MonsterGenerateTime,buf);
fprintf(handle,"MonsterGenerateTime=%s\n",buf);

fprintf(handle,"MaximumNumberOfLoginAttempts=%d\n",config.MaximumNumberOfLoginAttempts);

fprintf(handle,"PointsForHero=%d\n",config.PointsForHero);
fprintf(handle,"PointsForWarrior=%d\n",config.PointsForWarrior);
fprintf(handle,"PointsForChampion=%d\n",config.PointsForChampion);
fprintf(handle,"PointsForSuperhero=%d\n",config.PointsForSuperhero);
fprintf(handle,"PointsForEnchanter=%d\n",config.PointsForEnchanter);
fprintf(handle,"PointsForSorceror=%d\n",config.PointsForSorceror);
fprintf(handle,"PointsForNecromancer=%d\n",config.PointsForNecromancer);
fprintf(handle,"PointsForLegend=%d\n",config.PointsForLegend);
fprintf(handle,"PointsForWizard=%d\n",config.PointsForWizard);

fclose(handle);
return(0);
}

void GetConfigurationInformation(CONFIG *buf) {
memcpy(buf,&config,sizeof(CONFIG));
}

void UpdateConfigurationInformation(CONFIG *buf) {
memcpy(&config,buf,sizeof(CONFIG));

ConfigurationUpdated=FALSE;
}

sqlite3 *GetDatabaseHandle(void) {
return(DatabaseHandle);
}

void CloseDatabase(void) {
sqlite3_close(DatabaseHandle);
}

char *GetDatabaseFilename(void) {
return(DatabaseFilename);
}

bool GetConfigurationUpdatedFlag(void) {
return(ConfigurationUpdated);
}

void SetConfigurationUpdatedFlag(bool WasUpdated) {
ConfigurationUpdated=WasUpdated;
}

