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
char *SpellsConfigurationFile="config/spells.mud";
char *SpellHasKilled="and have killed it";

int CastSpell(user *currentuser,char *spellname,char *target) {
room *currentroom;
user *usernext;
monster *monsternext;
char *SpellMessage[BUF_SIZE];
int SpellFound;
int HitPoints;
int MonsterCount;
CONFIG config;
spell *spellnext;

GetConfigurationInformation(&config);

if(target == NULL) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

currentroom=currentuser->roomptr;

/*
* find spell
*/

spellnext=spells;
while(spellnext != NULL) {

	if(strcmp(spellnext->name,spellname) == 0) {			/* found spell */
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
	
		SpellFound=TRUE;			/* found spell */
		break;
	}

	spellnext=spellnext->next;
}

if(SpellFound == FALSE) {	/* no spell */
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

while(usernext != NULL) {
	if((regexp(usernext->name,target) == TRUE) && (usernext->loggedin == TRUE) && (usernext->room == currentuser->room)) {
		if(config.allowplayerkilling == FALSE) {		/* can't kill player */
			SetLastError(currentuser,SPELL_HAVEN);
		}

		if(usernext->status > WIZARD) {		/* if not user, cast spell */
			if(usernext->gender == MALE) {
				sprintf(SpellMessage,"%s casts a spell on %s the %s but it just bounces off with no effect\r\n",currentuser->name,GetPointerToMaleTitles(currentuser->status));
				send(currentuser->handle,SpellMessage,strlen(SpellMessage),0);
				return(0);
			} 
			else
			{
				sprintf(SpellMessage,"%s casts a spell on %s the %s but it just bounces off with no effect\r\n",currentuser->name,GetPointerToFemaleTitles(currentuser->status));
				send(currentuser->handle,SpellMessage,strlen(SpellMessage),0);
				return(0);
			}

		}

		HitPoints=usernext->staminapoints-spellnext->damage;	/* deduct stamina points from user */

		UpdateUser(currentuser,usernext->name,"",0,0,"",0,HitPoints,0,0,"","",0); /* update user stamina points */ 
		UpdateUser(currentuser,currentuser->name,"",0,0,"",usernext->magicpoints-spellnext->spellpoints,currentuser->staminapoints,0,0,"","",0); /* ' update own spell points */

		sprintf(SpellMessage,"%s casts a %s on %s causing %d points of damage\r\n",currentuser->name,spellnext->message,usernext->name,spellnext->damage);
		send(currentuser->handle,SpellMessage,strlen(SpellMessage),0);

	}

	usernext=FindNextUser(usernext);		/* find next user */
}


/*
* cast spell on monster
*/

for(MonsterCount=0;MonsterCount < currentroom->monstercount;MonsterCount++) {
	if(regexp(target,currentroom->roommonsters[MonsterCount].name) == TRUE) {		/* found object */

	 /* calculate hit points */
	
		HitPoints=spellnext->spellpoints / (currentuser->status/2) + currentuser->race->strength+currentuser->race->luck;
		monsternext->stamina -= HitPoints;		/* deduct stamina points */

		sprintf(SpellMessage,"%s casts a %s on the %s and causes %d points of damage ",currentuser->name,spellnext->name,monsternext->name,HitPoints);

		send(currentuser->handle,SpellMessage,strlen(SpellMessage),0);

		if(monsternext->stamina <= 0) {		/* monster has been killed */
			send(currentuser->handle,SpellHasKilled,strlen(SpellHasKilled),0);
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

int LoadSpells(void) {
spell *spellnext;
FILE *handle;
int LineCount;
char *ConfigurationTokens[BUF_SIZE][BUF_SIZE];
char *LineBuffer[BUF_SIZE];
int ErrorCount=0;
char *CurrentDirectory[BUF_SIZE];

spellnext=spells;
LineCount=0;

handle=fopen(SpellsConfigurationFile,"rb");
if(handle == NULL) {																						/* couldn't open file */
	printf("\nmud: Can't open configuration file %s\n",SpellsConfigurationFile);
	return(-1);
}

while(!feof(handle)) {
	fgets(LineBuffer,BUF_SIZE,handle);		/* get and parse line */

	if((char) *LineBuffer == '#')  continue;		/* skip comments */
	if((char) *LineBuffer == '\n')  continue;		/* skip newline */

	RemoveNewLine(LineBuffer);		/* remove newline character */

	LineCount++;

	if(strlen(LineBuffer) < 2) continue;		/* skip blank line */

	TokenizeLine(LineBuffer,ConfigurationTokens,":\n");				/* tokenize line */

	if(strcmp(ConfigurationTokens[0],"begin_spell") == 0) {	/* end */

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


		strcpy(spellnext->name,ConfigurationTokens[1]);
		continue;			
	}

	if(strcmp(ConfigurationTokens[0],"spellpointsused") == 0) {	/* spell points used */
		spellnext->spellpoints=atoi(ConfigurationTokens[1]);
		continue;			
	}

	if(strcmp(ConfigurationTokens[0],"spelldamage") == 0) {	/* spell damage */
		spellnext->damage=atoi(ConfigurationTokens[1]);
		continue;			
	}

	if(strcmp(ConfigurationTokens[0],"spellmessage") == 0) {	/* spell message */
		strcpy(spellnext->message,ConfigurationTokens[1]);
		continue;			
	}

	if(strcmp(ConfigurationTokens[0],"spelllevel") == 0) {		/* level	*/
		spellnext->level=atoi(ConfigurationTokens[1]);
		continue;			
	}

	if(strcmp(ConfigurationTokens[0],"spellclass") == 0) {		/* class */
		strcpy(spellnext->class,ConfigurationTokens[1]);
		continue;			
	}

	if(strcmp(ConfigurationTokens[0],"end") == 0) continue;

	if(strcmp(ConfigurationTokens[0],"#") == 0) continue;			

	printf("\nmud: %d: unknown configuration option %s in %s\n",LineCount,ConfigurationTokens[0],SpellsConfigurationFile);		/* unknown configuration option */
	ErrorCount++;
}

fclose(handle);
return(ErrorCount);
}

