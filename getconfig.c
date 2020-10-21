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
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

 /* load configuration */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "defines.h"

CONFIG config;


char *mudconf[BUF_SIZE];
char *isconf[BUF_SIZE];

char *isrel="/config/issue.mud";
char *mudrel="/config/mud.mud";

int getconfig(void) {
 FILE *handle;
 char *z[BUF_SIZE];
 int lc;
 char *ab[19][255];
 char c;
 char *b;
 int errorcount=0;
 int count;

getcwd(mudconf,BUF_SIZE);
strcat(mudconf,mudrel);

getcwd(isconf,BUF_SIZE);
strcat(isconf,isrel);


/* load general configuration */

 printf("Loading configuration...");

 handle=fopen(mudconf,"rb");
 if(handle == NULL) {                                           /* couldn't open file */
  printf("mud: Can't open configuration file %s\n",mudconf);

  exit(NOCONFIGFILE);
 }

 lc=0;

 while(!feof(handle)) {
  fgets(z,BUF_SIZE,handle);			/* get data */

  if(strlen(z) <= 1) continue;                  /* skip blank line */

  b=z;
  c=*b;
  if(c == '#')  continue;		/* skip comments */
  if(c == '\n')  continue;		/* skip newline */

  b=z;
  b=b+strlen(z);
  b--;

  if(*b == '\n') *b=0;
  b--;
  if(*b == '\r') *b=0;

  tokenize_line(z,ab,"=");			/* tokenize line */

  lc++;                                          /* line count */


  if(strcmp(ab[0],"server") == 0) {
   strcpy(config.mudserver,ab[1]);
   continue;
  }

  if(strcmp(ab[0],"port") == 0) {
   config.mudport=atoi(ab[1]);   
   continue;
  }

  if(strcmp(ab[0],"objectresettime") == 0) {        /* how often to reset objects */
   config.objectresettime=getvaluefromtimestring(ab[1]);
   continue;
  }

  if(strcmp(ab[0],"databasesavetime") == 0) {	   /* how often to save database */
   config.databaseresettime=getvaluefromtimestring(ab[1]);
   continue;
  }

  if(strcmp(ab[0],"userresettime") == 0) {                /* how often to reset users */
   config.userresettime=getvaluefromtimestring(ab[1]);
   continue;
  }

  if(strcmp(ab[0],"configsavetime") == 0) {		/* backup database before save */
   config.configsavetime=getvaluefromtimestring(ab[1]);
   continue;
  }

  if(strcmp(ab[0],"databasebackup") == 0) {		/* backup database before save */
   config.databasebackup=-1;

   if(strcmp(ab[1],"true") == 0) config.databasebackup=TRUE;
   if(strcmp(ab[1],"false") == 0) config.databasebackup=FALSE;

   if(config.databasebackup == -1) printf("mud: %d: value must be true or false\n",lc);	/* invalid option */
   continue;
  }

   if(strcmp(ab[0],"allowplayerkilling") == 0) {		/* allow player killing */
    config.allowplayerkilling=-1;

    if(strcmp(ab[1],"true") == 0) config.allowplayerkilling=TRUE;
    if(strcmp(ab[1],"false") == 0) config.allowplayerkilling=FALSE;

    if(config.allowplayerkilling == -1) printf("mud: %d: value must be true or false\n",lc);	/* invalid option */
    continue;
   }

   if(strcmp(ab[0],"allownewaccounts") == 0) {		/* allow new accounts */
    config.allownewaccounts=-1;

    if(strcmp(ab[1],"true") == 0) config.allownewaccounts=TRUE;
    if(strcmp(ab[1],"false") == 0) config.allownewaccounts=FALSE;
 
    if(config.allownewaccounts == -1) printf("mud: %d: value must be true or false\n",lc);	/* invalid option */
    continue;
   }

   if(strcmp(ab[0],"monsterresettime") == 0) {		/* how often to reset monsters */
    config.monsterresettime=getvaluefromtimestring(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"banresettime") == 0) {		/* how often to reset monsters */
    config.banresettime=getvaluefromtimestring(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforwarrior") == 0) {
    config.pointsforwarrior=atoi(ab[1]);	/* points for levels */
    continue;
   }

   if(strcmp(ab[0],"pointsforhero") == 0) {
    config.pointsforhero=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforchampion") == 0) {
    config.pointsforchampion=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforsuperhero") == 0) {
    config.pointsforsuperhero=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforenchanter") == 0) {
    config.pointsforenchanter=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforsorceror") == 0) {
    config.pointsforsorceror=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsfornecromancer") == 0) {
    config.pointsfornecromancer=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforlegend") == 0) {
    config.pointsforlegend=atoi(ab[1]);
    continue;
   }
   
   if(strcmp(ab[0],"pointsforwizard") == 0) {
    config.pointsforwizard=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"#") == 0) continue;		/* comment */

   printf("\nmud: %d: unknown configuration option %s in %s\n",lc,ab[0],mudconf);		/* unknown configuration option */
   errorcount++;
 }

 fclose(handle);
 if(errorcount == 0) {
  printf("ok\n");
 }
 else
 {
  printf("\n");
 }

/* load MUD database */

printf("Loading database...");
errorcount += loaddatabase();
if(errorcount == 0) {
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
 errorcount += loadspells();

if(errorcount == 0) {
 printf("ok\n");
}
else
{
 printf("\n");
}

/*
 * load monsters
 */

 lc=0;

 printf("Loading monsters...");
 errorcount += loadmonsters();

if(errorcount == 0) {
  printf("ok\n");
}
else
{
 printf("\n");
}

/*
 * load races+
 */
printf("Loading races...");

errorcount += loadraces();
if(errorcount == 0) {
 printf("ok\n");
}
else
{
 printf("\n");
}

printf("Loading classes...");
errorcount += loadclasses();
if(errorcount == 0) {
 printf("ok\n");
}
else
{
 printf("\n");
}

printf("Loading message...");

/* LOAD issue message */
handle=fopen(isconf,"rb");
if(handle == NULL) {                                           /* couldn't open file */
 printf("mud: Can't open configuration file %s\n",isconf);
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
errorcount += loadusers();
if(errorcount == 0) {
 printf("ok\n");
}
else
{
 printf("\n");
}

printf("Creating monsters...");
createmonster();
printf("ok\n");


/*
 * load banlist
 */

printf("Loading bans...");
loadbans();
printf("ok\n");

printf("Creating objects...");
resetobjects();
printf("ok\n");

if(errorcount > 0) {			/* errors */
 printf("%d errors\n",errorcount);
 exit(CONFIG_ERROR);
}

return;
}

int updateconfiguration(void) {
 FILE *handle;
 char *buf[BUF_SIZE];

 handle=fopen(mudconf,"w");
 if(handle == NULL) return;                                          /* couldn't open file */


 fprintf(handle,"server=%s\n",config.mudserver);
 fprintf(handle,"port=%d\n",config.mudport);

 memset(buf,0,BUF_SIZE);

 createtimestring(config.objectresettime,buf);
 fprintf(handle,"objectresettime=%s\n",buf);

 memset(buf,0,BUF_SIZE);

 createtimestring(config.databaseresettime,buf);
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

 createtimestring(config.monsterresettime,buf);

 fprintf(handle,"monsterresettime=%s\n",buf);

 memset(buf,0,BUF_SIZE);
 createtimestring(config.banresettime,buf);

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
return;
}

int getconfigurationinformation(CONFIG *buf) {
 memcpy(buf,&config,sizeof(CONFIG));
}

int updateconfigurationinformation(CONFIG *buf) {
 memcpy(&config,buf,sizeof(CONFIG));
}

