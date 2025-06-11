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

#include "bool.h"
#include "errors.h"
#include "config.h"

CONFIG config;
char *BannerFile="config/issue.mud";
char *ConfigurationFile="config/mud.mud";
char *MustBeTrueOrFalse="mud: %d: value must be true or false\n";

int GetConfiguration(void) {
FILE *handle;
char *LineBuffer[BUF_SIZE];
int LineCount;
char *LineTokens[BUF_SIZE][BUF_SIZE];
int ErrorCount=0;


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

	if(strcmp(LineTokens[0],"server") == 0) {
		strcpy(config.mudserver,LineTokens[1]);
	}
	else if(strcmp(LineTokens[0],"port") == 0) {
		config.mudport=atoi(LineTokens[1]);   
	}
	else if(strcmp(LineTokens[0],"objectresettime") == 0) {        /* how often to reset objects */
		config.objectresettime=GetValueFromTimeString(LineTokens[1]);
	}
	else if(strcmp(LineTokens[0],"databasesavetime") == 0) {	   /* how often to save database */
		config.databaseresettime=GetValueFromTimeString(LineTokens[1]);
	}
	else if(strcmp(LineTokens[0],"userresettime") == 0) {                /* how often to reset users */
		config.userresettime=GetValueFromTimeString(LineTokens[1]);
	}
	else if(strcmp(LineTokens[0],"configsavetime") == 0) {		/* backup database before save */
		config.configsavetime=GetValueFromTimeString(LineTokens[1]);
	}
	else if(strcmp(LineTokens[0],"databasebackup") == 0) {		/* backup database before save */
		config.databasebackup=-1;

		if(strcmp(LineTokens[1],"true") == 0) config.databasebackup=TRUE;
		if(strcmp(LineTokens[1],"false") == 0) config.databasebackup=FALSE;

		if(config.databasebackup == -1) printf(MustBeTrueOrFalse,LineCount);	/* invalid option */

	}
	else if(strcmp(LineTokens[0],"allowplayerkilling") == 0) {		/* allow player killing */
		config.allowplayerkilling=-1;

		if(strcmp(LineTokens[1],"true") == 0) config.allowplayerkilling=TRUE;
		if(strcmp(LineTokens[1],"false") == 0) config.allowplayerkilling=FALSE;

		if(config.allowplayerkilling == -1) printf(MustBeTrueOrFalse,LineCount);	/* invalid option */
	 }
	 else if(strcmp(LineTokens[0],"allownewaccounts") == 0) {		/* allow new accounts */
		config.allownewaccounts=-1;

		if(strcmp(LineTokens[1],"true") == 0) config.allownewaccounts=TRUE;
		if(strcmp(LineTokens[1],"false") == 0) config.allownewaccounts=FALSE;

		if(config.allownewaccounts == -1) printf(MustBeTrueOrFalse,LineCount);	/* invalid option */
	 }
	 else if(strcmp(LineTokens[0],"monsterresettime") == 0) {		/* how often to reset monsters */
		config.monsterresettime=GetValueFromTimeString(LineTokens[1]);
	 }
	 else if(strcmp(LineTokens[0],"banresettime") == 0) {		/* how often to reset monsters */
		config.banresettime=GetValueFromTimeString(LineTokens[1]);
	 }
	 else if(strcmp(LineTokens[0],"maxobjectsperroom") == 0) {
		config.roomobjectnumber=atoi(LineTokens[1]);
	 }
	 else if(strcmp(LineTokens[0],"pointsforwarrior") == 0) {
		config.pointsforwarrior=atoi(LineTokens[1]);	/* points for levels */
	 }
	 else if(strcmp(LineTokens[0],"pointsforhero") == 0) {
		config.pointsforhero=atoi(LineTokens[1]);
	 }
	 else if(strcmp(LineTokens[0],"pointsforchampion") == 0) {
		config.pointsforchampion=atoi(LineTokens[1]);
	 }
	 else if(strcmp(LineTokens[0],"pointsforsuperhero") == 0) {
		config.pointsforsuperhero=atoi(LineTokens[1]);
	 }
	 else if(strcmp(LineTokens[0],"pointsforenchanter") == 0) {
		config.pointsforenchanter=atoi(LineTokens[1]);
	 }
	 else if(strcmp(LineTokens[0],"pointsforsorceror") == 0) {
		config.pointsforsorceror=atoi(LineTokens[1]);
	 }
	 else if(strcmp(LineTokens[0],"pointsfornecromancer") == 0) {
		config.pointsfornecromancer=atoi(LineTokens[1]);
	 }
	 else if(strcmp(LineTokens[0],"pointsforlegend") == 0) {
		config.pointsforlegend=atoi(LineTokens[1]);
	 }
	 else if(strcmp(LineTokens[0],"pointsforwizard") == 0) {
		config.pointsforwizard=atoi(LineTokens[1]);
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

printf("Loading database...");

ErrorCount += LoadDatabase();

if(ErrorCount == 0) {
	printf("ok\n");
}
else
{
	printf("\n");
}

/*
* load spells
*/

printf("Loading spells...");
ErrorCount += LoadSpells();

if(ErrorCount == 0) {
	printf("ok\n");
}
else
{
printf("\n");
}

/*
* load monsters
*/

LineCount=0;

printf("Loading monsters...");
ErrorCount += LoadMonsters();

if(ErrorCount == 0) {
	printf("ok\n");
}
else
{
	printf("\n");
}

/*
* load races
*/
printf("Loading races...");

ErrorCount += LoadRaces();
if(ErrorCount == 0) {
	printf("ok\n");
}
else
{
	printf("\n");
}

printf("Loading classes...");

ErrorCount += LoadClasses();

if(ErrorCount == 0) {
	printf("ok\n");
}
else
{
	printf("\n");
}

printf("Loading banner message...");

/* LOAD issue message */
handle=fopen(BannerFile,"rb");
if(handle == NULL) {                                           /* couldn't open file */
	printf("mud: Can't open configuration file %s\n",BannerFile);
	exit(NOCONFIGFILE);
}

fseek(handle,0,SEEK_END);		/* get file size */
config.issuecount=ftell(handle);
fseek(handle,0,SEEK_SET);

config.isbuf=calloc(1,config.issuecount);
if(config.isbuf == NULL) {			/* can't allocate */
	perror("mud:");
	exit(NOMEM);
}

fread(config.isbuf,1,config.issuecount,handle);		/* read data */

fclose(handle);

printf("ok\n");

printf("Loading users...");

ErrorCount += LoadUsers();

if(ErrorCount == 0) {
	printf("ok\n");
}
else
{
	printf("\n");
}

printf("Generating monsters...");
GenerateMonsters();
printf("ok\n");


/*
* load ban list
*/

printf("Loading bans...");
LoadBans();
printf("ok\n");

printf("Creating objects...");

GenerateObjects();
printf("ok\n");

if(ErrorCount > 0) {			/* errors */
	printf("%d errors\n",ErrorCount);
	exit(CONFIG_ERROR);
}

return(0);
}

int updateconfiguration(void) {
FILE *handle;
char *buf[BUF_SIZE];

handle=fopen(ConfigurationFile,"w");
if(handle == NULL) return(-1);                                        /* couldn't open file */


fprintf(handle,"server=%s\n",config.mudserver);
fprintf(handle,"port=%d\n",config.mudport);

memset(buf,0,BUF_SIZE);

CreateTimeString(config.objectresettime,buf);
fprintf(handle,"objectresettime=%s\n",buf);

memset(buf,0,BUF_SIZE);

CreateTimeString(config.databaseresettime,buf);
fprintf(handle,"databasesavetime=%s\n",buf);

fputs("databasebackup=",handle);
if(config.databasebackup == TRUE) {
	fputs("true\n",handle);
}
else
{
	fputs("false\n",handle);
}

fputs("allowplayerkilling=",handle);
if(config.allowplayerkilling == TRUE) {
	fputs("true\n",handle);
}
else
{
	fputs("false\n",handle);
}


fputs("allownewaccounts=",handle);
if(config.allownewaccounts== TRUE) {
	fputs("true\n",handle);
}
else
{
	fputs("false\n",handle);
}

memset(buf,0,BUF_SIZE);

CreateTimeString(config.monsterresettime,buf);

fprintf(handle,"monsterresettime=%s\n",buf);

memset(buf,0,BUF_SIZE);
CreateTimeString(config.banresettime,buf);

fprintf(handle,"banresettime=%s\n",buf);
fprintf(handle,"pointsforhero=%d\n",config.pointsforhero);
fprintf(handle,"pointsforwarrior=%d\n",config.pointsforwarrior);
fprintf(handle,"pointsforchampion=%d\n",config.pointsforchampion);
fprintf(handle,"pointsforsuperhero=%d\n",config.pointsforsuperhero);
fprintf(handle,"pointsforenchanter=%d\n",config.pointsforenchanter);
fprintf(handle,"pointsforsorceror=%d\n",config.pointsforsorceror);
fprintf(handle,"pointsfornecromancer=%d\n",config.pointsfornecromancer);
fprintf(handle,"pointsforlegend=%d\n",config.pointsforlegend);
fprintf(handle,"pointsforwizard=%d\n",config.pointsforwizard);

fclose(handle);
return(0);
}

void GetConfigurationInformation(CONFIG *buf) {
memcpy(buf,&config,sizeof(CONFIG));
}

void UpdateConfigurationInformation(CONFIG *buf) {
memcpy(&config,buf,sizeof(CONFIG));
}

