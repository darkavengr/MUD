/*
* cast a spell on someone or a monster
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sqlite3.h>

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

char *SpellHasKilled="and have killed it";

int CastSpell(user *currentuser,char *spellname,char *target) {
room *currentroom;
user *UserPtr;
monster *monsternext;
char *SpellMessage[BUF_SIZE];
int HitPoints;
int MonsterCount;
CONFIG config;
sqlite3_stmt *SQLStatementHandle;
spell foundspell;
int returncode;

GetConfigurationInformation(&config);

if(target == NULL) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

currentroom=currentuser->roomptr;

/*
* find spell
*/

if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM SPELLS WHERE NAME=?;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	/* prepare SQL statement */
	SetLastError(currentuser,IO_ERROR);
	return(-1);
}

sqlite3_bind_text(SQLStatementHandle,1,spellname,strlen(spellname),NULL);		/* bind spell name to first parameter */

while(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {
	strncpy(foundspell.name,sqlite3_column_text(SQLStatementHandle,SPELL_NAME_COLUMN),BUF_SIZE);
	foundspell.spellpoints=sqlite3_column_int(SQLStatementHandle,SPELL_POINTS_COLUMN);
	foundspell.damage=sqlite3_column_int(SQLStatementHandle,SPELL_DAMAGE_COLUMN);
	strncpy(foundspell.message,sqlite3_column_text(SQLStatementHandle,SPELL_MESSAGE_COLUMN),BUF_SIZE);
	foundspell.level=sqlite3_column_int(SQLStatementHandle,SPELL_LEVEL_COLUMN);
	strncpy(foundspell.class,sqlite3_column_text(SQLStatementHandle,SPELL_CLASS_COLUMN),BUF_SIZE);
} 


if(returncode != SQLITE_DONE) {		/* spell not found */
	SetLastError(currentuser,SPELL_NOT_FOUND);
	return(-1);
}

sqlite3_finalize(SQLStatementHandle);

/*
* work out how many spell points are needed and display error message if not enough
*/

if(currentuser->magicpoints - foundspell.spellpoints <= 0) {
	if(currentuser->userlevel < WIZARD) {
		SetLastError(currentuser,INSUFFICIENT_MAGIC_POINTS);
		return(-1);
	}
}
	
/*
* casting spell on user
*/

if(((currentroom->attributes & ROOM_HAVEN) == TRUE) && (currentuser->userlevel < WIZARD)) {		/* can't put spells on users in haven rooms */
	SetLastError(currentuser,SPELL_HAVEN);
	return(-1);
}

UserPtr=FindFirstUser();		/* find first user */

while(UserPtr != NULL) {
	if((regexp(UserPtr->username,target) == TRUE) && (UserPtr->room == currentuser->room)) {
		if(config.AllowPlayerKilling == FALSE) {		/* can't kill player */
			SetLastError(currentuser,SPELL_HAVEN);
			return(-1);
		}

		if(UserPtr->userlevel > WIZARD) {		/* if not user, cast spell */
			if(UserPtr->gender == MALE) {
				sprintf(SpellMessage,"%s casts a spell on %s the %s but it just bounces off with no effect\r\n",currentuser->username,GetPointerToMaleTitles(currentuser->userlevel));
				send(currentuser->socket,SpellMessage,strlen(SpellMessage),0);
				return(0);
			} 
			else
			{
				sprintf(SpellMessage,"%s casts a spell on %s the %s but it just bounces off with no effect\r\n",currentuser->username,GetPointerToFemaleTitles(currentuser->userlevel));
				send(currentuser->socket,SpellMessage,strlen(SpellMessage),0);
				return(0);
			}

		}

		HitPoints=UserPtr->staminapoints-foundspell.damage;	/* deduct stamina points from user */

		/* update user stamina points */ 
		
		UpdateUser(UserPtr,UserPtr->username,UserPtr->username,UserPtr->password,UserPtr->homeroom,UserPtr->userlevel,UserPtr->description,UserPtr->magicpoints,UserPtr->staminapoints - HitPoints,UserPtr->experiencepoints,UserPtr->gender,&UserPtr->race,&UserPtr->userclass,UserPtr->flags);

		/* update own spell points */

		UpdateUser(currentuser,currentuser->username,currentuser->username,currentuser->password,currentuser->homeroom,currentuser->userlevel,currentuser->description,currentuser->magicpoints-foundspell.spellpoints,currentuser->staminapoints,currentuser->experiencepoints,currentuser->gender,&currentuser->race,&currentuser->userclass,currentuser->flags);

		sprintf(SpellMessage,"%s casts a %s on %s causing %d points of damage\r\n",currentuser->username,foundspell.message,UserPtr->username,foundspell.damage);
		send(currentuser->socket,SpellMessage,strlen(SpellMessage),0);

	}

	UserPtr=FindNextUser(UserPtr);		/* find next user */
}


/*
* cast spell on monster
*/

for(MonsterCount=0;MonsterCount < currentroom->monstercount;MonsterCount++) {
	if(regexp(target,currentroom->roommonsters[MonsterCount].name) == TRUE) {		/* found object */

	 /* calculate hit points */
	
		HitPoints=foundspell.spellpoints / (currentuser->userlevel/2) + currentuser->race.strength+currentuser->race.luck;
		monsternext->stamina -= HitPoints;		/* deduct stamina points */

		sprintf(SpellMessage,"%s casts a %s on the %s and causes %d points of damage ",currentuser->username,foundspell.name,monsternext->name,HitPoints);

		send(currentuser->socket,SpellMessage,strlen(SpellMessage),0);

		if(monsternext->stamina <= 0) {		/* monster has been killed */
			send(currentuser->socket,SpellHasKilled,strlen(SpellHasKilled),0);
		}
		else
		{
			send(currentuser->socket,"\r\n",2,0);
		}

		monsternext->last=monsternext->next;		/* remove monster */
		free(monsternext);
	}

}

return(0);
}

