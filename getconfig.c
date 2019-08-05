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

#include "defs.h"


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

  if(*b == '\n') strtrunc(z,1);
  b--;
  if(*b == '\r') strtrunc(z,1);

  tokenize_line(z,ab,"=");			/* tokenize line */

  lc++;                                          /* line count */


  if(strcmp(ab[0],"server") == 0) {
   strcpy(mudserver,ab[1]);
   continue;
  }

  if(strcmp(ab[0],"port") == 0) {
   mudport=atoi(ab[1]);   
   continue;
  }

  if(strcmp(ab[0],"objectresettime") == 0) {        /* how often to reset objects */
   objectresettime=getvaluefromtimestring(ab[1]);
   continue;
  }

  if(strcmp(ab[0],"databasesavetime") == 0) {	   /* how often to save database */
   databaseresettime=getvaluefromtimestring(ab[1]);
   continue;
  }

  if(strcmp(ab[0],"userresettime") == 0) {                /* how often to reset users */
   userresettime=getvaluefromtimestring(ab[1]);
   continue;
  }

  if(strcmp(ab[0],"configsavetime") == 0) {		/* backup database before save */
   configsavetime=getvaluefromtimestring(ab[1]);
   continue;
  }

  if(strcmp(ab[0],"databasebackup") == 0) {		/* backup database before save */
   databasebackup=-1;

   if(strcmp(ab[1],"true") == 0) databasebackup=TRUE;
   if(strcmp(ab[1],"false") == 0) databasebackup=FALSE;

   if(databasebackup == -1) printf("mud: %d: value must be true or false\n",lc);	/* invalid option */
   continue;
  }

   if(strcmp(ab[0],"allowplayerkilling") == 0) {		/* allow player killing */
    allowplayerkilling;

    if(strcmp(ab[1],"true") == 0) allowplayerkilling=TRUE;
    if(strcmp(ab[1],"false") == 0) allowplayerkilling=FALSE;

    if(allowplayerkilling == -1) printf("mud: %d: value must be true or false\n",lc);	/* invalid option */
    continue;
   }

   if(strcmp(ab[0],"allownewaccounts") == 0) {		/* allow new accounts */
    allownewaccounts=-1;

    if(strcmp(ab[1],"true") == 0) allownewaccounts=TRUE;
    if(strcmp(ab[1],"false") == 0) allownewaccounts=FALSE;
 
    if(allownewaccounts == -1) printf("mud: %d: value must be true or false\n",lc);	/* invalid option */
    continue;
   }

   if(strcmp(ab[0],"monsterresettime") == 0) {		/* how often to reset monsters */
    monsterresettime=getvaluefromtimestring(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"banresettime") == 0) {		/* how often to reset monsters */
    banresettime=getvaluefromtimestring(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforwarrior") == 0) {
    pointsforwarrior=atoi(ab[1]);	/* points for levels */
    continue;
   }

   if(strcmp(ab[0],"pointsforhero") == 0) {
    pointsforhero=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforchampion") == 0) {
    pointsforchampion=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforsuperhero") == 0) {
    pointsforsuperhero=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforenchanter") == 0) {
    pointsforenchanter=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforsorceror") == 0) {
    pointsforsorceror=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsfornecromancer") == 0) {
    pointsfornecromancer=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"pointsforlegend") == 0) {
    pointsforlegend=atoi(ab[1]);
    continue;
   }
   
   if(strcmp(ab[0],"pointsforwizard") == 0) {
    pointsforwizard=atoi(ab[1]);
    continue;
   }

   if(strcmp(ab[0],"#") == 0) continue;		/* comment */

   printf("mud: %d: unknown configuration option %s in %s\n",lc,ab[0],mudconf);		/* unknown configuration option */
   errorcount++;
 }

 fclose(handle);
 printf("ok\n");

/* load MUD database */

 printf("Loading database...");
 loaddatabase();

// resetobjects();
printf("ok\n");


/*
 * load spells
 */

 printf("Loading spells...");
 loadspells();
printf("ok\n");

/*
 * load monsters
 */

 lc=0;

 printf("Loading monsters...");
 loadmonsters();
printf("ok\n");

/*
 * load races+
 */
printf("Loading races...");
loadraces();
printf("ok\n");

printf("Loading classes...");
loadclasses();

printf("ok\n");

printf("Loading message...");

/* LOAD issue message */
handle=fopen(isconf,"rb");
if(handle == NULL) {                                           /* couldn't open file */
 printf("mud: Can't open configuration file %s\n",isconf);
 exit(NOCONFIGFILE);
}

fseek(handle,0,SEEK_END);		/* get file size */
issuecount=ftell(handle);
fseek(handle,0,SEEK_SET);

isbuf=calloc(1,issuecount);
if(isbuf == NULL) {			/* can't allocate */
 printf("%s\n",mudnomem);
 exit(NOMEM);
}

fread(isbuf,1,issuecount,handle);		/* read data */
fclose(handle);

printf("ok\n");

printf("Loading users...");
loadusers();
printf("ok\n");

printf("mud: creating monsters...");
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
 printf("mud: %d errors\n",errorcount);
 exit(CONFIG_ERROR);
}

return;
}

int updateconfiguration(void) {
 FILE *handle;
 char *buf[BUF_SIZE];

 handle=fopen(mudconf,"w");
 if(handle == NULL) return;                                          /* couldn't open file */


 fprintf(handle,"server=%s\n",mudserver);
 fprintf(handle,"port=%d\n",mudport);

 memset(buf,0,BUF_SIZE);

 createtimestring(objectresettime,buf);
 fprintf(handle,"objectresettime=%s\n",buf);

 memset(buf,0,BUF_SIZE);

 createtimestring(databaseresettime,buf);
 fprintf(handle,"databasesavetime=%s\n",buf);

 fputs("databasebackup=",handle);
 if(databasebackup == TRUE) {
  fputs("true\n",handle);
 }
 else
 {
  fputs("false\n",handle);
 }

 fputs("allowplayerkilling=",handle);
 if(allowplayerkilling == TRUE) {
  fputs("true\n",handle);
 }
 else
 {
  fputs("false\n",handle);
 }


 fputs("allownewaccounts=",handle);
 if(allownewaccounts== TRUE) {
  fputs("true\n",handle);
 }
 else
 {
  fputs("false\n",handle);
 }

 memset(buf,0,BUF_SIZE);

 createtimestring(monsterresettime,buf);

 fprintf(handle,"monsterresettime=%s\n",buf);

 memset(buf,0,BUF_SIZE);
 createtimestring(banresettime,buf);

 fprintf(handle,"banresettime=%s\n",buf);
 fprintf(handle,"pointsforhero=%d\n",pointsforhero);
 fprintf(handle,"pointsforwarrior=%d\n",pointsforwarrior);
 fprintf(handle,"pointsforchampion=%d\n",pointsforchampion);
 fprintf(handle,"pointsforsuperhero=%d\n",pointsforsuperhero);
 fprintf(handle,"pointsforenchanter=%d\n",pointsforenchanter);
 fprintf(handle,"pointsforsorceror=%d\n",pointsforsorceror);
 fprintf(handle,"pointsfornecromancer=%d\n",pointsfornecromancer);
 fprintf(handle,"pointsforlegend=%d\n",pointsforlegend);
 fprintf(handle,"pointsforwizard=%d\n",pointsforwizard);

 fclose(handle);
return;
}

