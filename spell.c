/*
* cast a spell on someone or a monster
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef __linux__
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
#include "winsock.h"
#endif

#include "bool.h"
#include "spell.h"
#include "race.h"
#include "errors.h"
#include "user.h"
#include "config.h"
		
spell *spells=NULL;
char *spellconf[BUF_SIZE];
char *spellsrel="/config/spells.mud";
char *spellhaskilled="and has killed it";

int castspell(user *currentuser,char *s,char *t) {
room *currentroom;
spell *spellnext;
user *usernext;
race *userrace;
monster *monsternext;
char *buf[BUF_SIZE];
int spellfound;
int pointsx;
int count;
CONFIG config;

if(t == NULL) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

getconfigurationinformation(&config);

currentroom=currentuser->roomptr;
userrace=currentuser->race;		/* get race */

/*
* find spell
*/

spellnext=spells;
while(spellnext != NULL) {

	if(strcmp(spellnext->name,s) == 0) {			/* found spell */

	if((currentuser->status < spellnext->level) && currentuser->status < WIZARD) {		/* spell required a higher level user */
		SetLastError(currentuser,SPELL_LEVEL_USER);
		return(-1);
	}

/*
* work out how many spell points are needed and display error message if not enough
*/

	if(currentuser->magicpoints-spellnext->spellpoints <= 0) {

		if(currentuser->status < WIZARD) {
			SetLastError(currentuser,INSUFFICIENT_MAGIC_POINTS);
			return(-1);
		}
	}
	
	spellfound=TRUE;			/* found spell */
	break;
}

spellnext=spellnext->next;
}

if(spellfound == FALSE) {	/* no spell */
	SetLastError(currentuser,SPELL_NOT_FOUND);
	return(-1);
}

/*
* casting spell on user
*/

if((currentroom->attr & ROOM_HAVEN) == TRUE && currentuser->status < WIZARD) {		/* can't put spells on users in haven rooms */
	SetLastError(currentuser,SPELL_HAVEN);
	return(-1);
}

usernext=FindFirstUser();		/* find first user */

usernext=FindFirstUser();		/* find first user */

while(usernext != NULL) {
	if((regexp(usernext->name,t) == TRUE) && (usernext->loggedin == TRUE) && (usernext->room == currentuser->room)) {
		if(config.allowplayerkilling == FALSE) {		/* can't kill player */
			SetLastError(currentuser,SPELL_HAVEN);
		}

		if(usernext->status > WIZARD) {		/* if not user, cast spell */
			if(usernext->gender == MALE) {
				sprintf(buf,"%s casts a spell on %s the %s but it just bounces off with no effect\r\n",currentuser->name,GetPointerToMaleTitles(currentuser->status));
				send(currentuser->handle,buf,strlen(buf),0);
				return(0);
			} 
			else
			{
				sprintf(buf,"%s casts a spell on %s the %s but it just bounces off with no effect\r\n",currentuser->name,GetPointerToFemaleTitles(currentuser->status));
				send(currentuser->handle,buf,strlen(buf),0);
				return(0);
			}

		}

		pointsx=usernext->staminapoints-spellnext->damage;	/* deduct stamina points from user */

		updateuser(currentuser,usernext->name,"",0,0,"",0,pointsx,0,0,"","",0); /* update user stamina points */ 
		updateuser(currentuser,currentuser->name,"",0,0,"",usernext->magicpoints-spellnext->spellpoints,currentuser->staminapoints,0,0,"","",0); /* ' update own spell points */

		sprintf(buf,"%s casts a %s on %s causing %d points of damage\r\n",currentuser->name,spellnext->message,usernext->name,spellnext->damage);
		send(currentuser->handle,buf,strlen(buf),0);

	}

	usernext=FindNextUser(usernext);		/* find next user */
}


/*
* cast spell on monster
*/

for(count=0;count<currentroom->monstercount;count++) {
	if(regexp(t,currentroom->roommonsters[count].name) == TRUE) {		/* found object */

	 /* calculate hit points */
	
		pointsx=spellnext->spellpoints / (currentuser->status/2) + userrace->strength+userrace->luck;
		monsternext->stamina -= pointsx;		/* deduct stamina points */

		sprintf(buf,"%s casts a %s on the %s and causes %d points of damage ",currentuser->name,spellnext->name,monsternext->name,pointsx);

		send(currentuser->handle,buf,strlen(buf),0);

		if(monsternext->stamina <= 0) {		/* monster has been killed */
			send(currentuser->handle,spellhaskilled,strlen(spellhaskilled),0);
		}
		else
		{
			send(currentuser->handle,"\r\n",2,0);
		}

		monsternext->last=monsternext->next;		/* remove monster */
		free(monsternext);
	}

}

return(0);
}

int loadspells(void) {
spell *spellnext;
FILE *handle;
int lc;
char *configuration_line[BUF_SIZE][BUF_SIZE];
char *linebuffer[BUF_SIZE];
int errorcount=0;
char *currentdirectory[BUF_SIZE];

getcwd(currentdirectory,BUF_SIZE);
sprintf(spellconf,"%s/%s",currentdirectory,spellsrel);

spellnext=spells;
lc=0;

handle=fopen(spellconf,"rb");
if(handle == NULL) {																						/* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",spellconf);
	return(-1);
}

while(!feof(handle)) {
	fgets(linebuffer,BUF_SIZE,handle);		/* get and parse line */

	if((char) *linebuffer == '#')  continue;		/* skip comments */
	if((char) *linebuffer == '\n')  continue;		/* skip newline */

	removenewline(linebuffer);		/* remove newline character */

	lc++;

	if(strlen(linebuffer) < 2) continue;		/* skip blank line */

	tokenize_line(linebuffer,configuration_line,":\n");				/* tokenize line */

	if(strcmp(configuration_line[0],"begin_spell") == 0) {	/* end */

		if(spells == NULL) {			/* first room */
			spells=calloc(1,sizeof(spell));
			if(spells == NULL) {
				perror("\nmud:");
				exit(NOMEM);
			}

			spellnext=spells;
		}
		else
		{
			spellnext->next=calloc(1,sizeof(spell));
			spellnext=spellnext->next;

			if(spellnext == NULL) {
				perror("\nmud:");
				exit(NOMEM);
			}
		}


		strcpy(spellnext->name,configuration_line[1]);
		continue;			
	}

	if(strcmp(configuration_line[0],"spellpointsused") == 0) {	/* spell points used */
		spellnext->spellpoints=atoi(configuration_line[1]);
		continue;			
	}

	if(strcmp(configuration_line[0],"spelldamage") == 0) {	/* spell damage */
		spellnext->damage=atoi(configuration_line[1]);
		continue;			
	}

	if(strcmp(configuration_line[0],"spellmessage") == 0) {	/* spell message */
		strcpy(spellnext->message,configuration_line[1]);
		continue;			
	}

	if(strcmp(configuration_line[0],"spelllevel") == 0) {		/* level	*/
		spellnext->level=atoi(configuration_line[1]);
		continue;			
	}

	if(strcmp(configuration_line[0],"spellclass") == 0) {		/* class */
		strcpy(spellnext->class,configuration_line[1]);
		continue;			
	}

	if(strcmp(configuration_line[0],"end") == 0) continue;

	if(strcmp(configuration_line[0],"#") == 0) continue;			

	printf("\nmud: %d: unknown configuration option %s in %s\n",lc,configuration_line[0],spellconf);		/* unknown configuration option */
	errorcount++;
}

fclose(handle);
return(errorcount);
}

