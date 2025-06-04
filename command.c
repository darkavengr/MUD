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

/* command interpreter */

#include <stdio.h>
#include <string.h>

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

#include "version.h"
#include "bool.h"
#include "directions.h"
#include "class.h"
#include "race.h"
#include "errors.h"
#include "room.h"
#include "command.h"
#include "config.h"

extern char *roomnames[11];

struct {
user *user;
char *statement;
unsigned int (*call_statement)(user *,int,void *);		/* function pointer */
} statements[] = { {  NULL,"NORTH",&north_statement },\
		 {  NULL,"N",&north_statement },\
		 {  NULL,"NORTHWEST",&northwest_statement },\
		 {  NULL,"NW",&northwest_statement },\
		 {  NULL,"SOUTH",&south_statement },\
		 {  NULL,"S",&south_statement },\
		 {  NULL,"SOUTHEAST",&southeast_statement },\
		 {  NULL,"SE",&southeast_statement },\
		 {  NULL,"EAST",&east_statement },\
		 {  NULL,"E",&east_statement },\
		 {  NULL,"WEST",&west_statement },\
		 {  NULL,"W",&west_statement },\
		 {  NULL,"SOUTHWEST",&southwest_statement },\
		 {  NULL,"SW",&southwest_statement },\
		 {  NULL,"UP",&up_statement },\
		 {  NULL,"U",&up_statement },\
		 {  NULL,"DOWN",&down_statement },\
		 {  NULL,"D",&down_statement },\
		 {  NULL,"LOOK",&look_statement },\
		 {  NULL,"WHO",&who_statement },\
		 {  NULL,"SAY",&say_statement },\
		 {  NULL,"WHISPER",&whisper_statement },\
		 {  NULL,":",&pose_statement },\
		 {  NULL,"POSE",&pose_statement },\
		 {  NULL,"HOME",&home_statement },\
		 {  NULL,"QUIT",&quit_statement },\
		 {  NULL,"VERSION",&version_statement },\
		 {  NULL,"DESCRIBE",&describe_statement },\
		 {  NULL,"GET",&get_statement },\
		 {  NULL,"DROP",&drop_statement },\
		 {  NULL,"HELP",&help_statement },\
		 {  NULL,"PASSWORD",&password_statement },\
		 {  NULL,"SPELL",&spell_statement },\
		 {  NULL,"F",&fight_statement },\
		 {  NULL,"SCORE",&score_statement },\
		 {  NULL,"INV",&inv_statement },\
		 {  NULL,"GIVE",give_statement },\
		 {  NULL,"XYZZY",&xyzzy_statement },\
		 {  NULL,"SETRACE",&setrace_statement },\
		 {  NULL,"SET",&set_statement },\
		 {  NULL,"SETHOME",&sethome_statement },\
		 {  NULL,"SETGENDER",&setgender_statement },\
		 {  NULL,"SETLEVEL",&setlevel_statement },\
		 {  NULL,"SETCLASSS",&setclass_statement },\
		 {  NULL,"SETXP",&setxp_statement },\
		 {  NULL,"SETMP",&setmp_statement },\
		 {  NULL,"SETSP",&setsp_statement },\
		 {  NULL,"BANIP",&banip_statement },\
		 {  NULL,"UNBAN",&unban_statement },\
		 {  NULL,"BAN",&ban_statement },\
		 {  NULL,"KILL",&kill_statement },\
		 {  NULL,"CREATE",&create_statement },\
		 {  NULL,"DELETE",&delete_statement },\
		 {  NULL,"RENAME",&rename_statement },\
		 {  NULL,"CHOWN",&chown_statement },\
		 {  NULL,"CHMOD",&chmod_statement },\
		 {  NULL,"COPY",&copy_statement },\
		 {  NULL,"MOVE",&move_statement },\
		 {  NULL,"DIG",&dig_statement },\
		 {  NULL,"FORCE",&force_statement },\
		 {  NULL,"LISTBAN",&listban_statement },\
		 {  NULL,"GO",&go_statement },\
		 {  NULL,"WALL",&wall_statement },\
		 {  NULL,"TAKE",&take_statement },\
		 {  NULL,"RELOAD",&reload_statement },\
		 {  NULL,"SHUTDOWN",&shutdown_statement },\
		 {  NULL,"ADDCLASS",&addclass_statement },\
		 {  NULL,"ADDRACE",&addrace_statement },\
		 {  NULL,"DROPDEAD",&dropdead_statement },\
		 {  NULL,"VISIBLE",&visible_statement },\
		 {  NULL,"INVISIBLE",&invisible_statement },\
		 {  NULL,"GAG",&gag_statement },\
		 {  NULL,"UNGAG",&ungag_statement },\
		 {  NULL,"SETEXIT",&setexit_statement },\
	         { NULL,NULL } };

int docommand(user *currentuser,char *s) {
char *command_tokens[BUF_SIZE][BUF_SIZE];
char *param[BUF_SIZE];
char *buf[BUF_SIZE];
int countx;
int count;
int x;
int tc;
int r;
char *b;
room *currentroom;
race race;
class class;
int whichroom;
int statementcount;

if(!*s) return;			/* no command */

currentroom=currentuser->roomptr;  

memset(command_tokens,0,10*BUF_SIZE);
tc=tokenize_line(s,command_tokens," ");			/* tokenize line */

strcpy(param,command_tokens[2]);
strcat(param," ");

for(count=3;count<tc;count++) {
	strcat(param,command_tokens[count]);
	strcat(param," ");
}

statementcount=0;

/* do statement */

do {
	if(statements[statementcount].statement == NULL) break;

	touppercase(command_tokens[0]);

	if(strcmp(statements[statementcount].statement,command_tokens[0]) == 0) {  
		statements[statementcount].call_statement(currentuser,tc,command_tokens);
		statementcount=0;
		return;
	}

	statementcount++;

} while(statements[statementcount].statement != NULL);

display_error(currentuser->handle,BAD_COMMAND);
return;
}

int north_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

go(currentuser,currentroom->exits[NORTH]);
return;
}

int south_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

go(currentuser,currentroom->exits[SOUTH]);
return;
}

int east_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

go(currentuser,currentroom->exits[EAST]);
return;
}

int west_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;
go(currentuser,currentroom->exits[NORTH]);
return;
}

int northwest_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

go(currentuser,currentroom->exits[NORTHWEST]);
return;
}

int southwest_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

go(currentuser,currentroom->exits[SOUTHWEST]);
return;
}

int southeast_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

go(currentuser,currentroom->exits[SOUTHEAST]);
return;
}

int northeast_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

go(currentuser,currentroom->exits[NORTHEAST]);
return;
}

int up_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

go(currentuser,currentroom->exits[UP]);
return;
}

int down_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

go(currentuser,currentroom->exits[DOWN]);
return;
}

int look_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
look(currentuser,command_tokens[1]);
return;
}

int who_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
who(currentuser,command_tokens[1]);
return;
}

int say_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
int count;
char *param[BUF_SIZE];
char *buf[BUF_SIZE];

if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

for(count=1;count<tc;count++) {
	strcat(param,command_tokens[count]);
	strcat(param," ");
}

if((currentuser->flags & USER_INVISIBLE) == 0) {
	sprintf(buf,"%s Says, \042%s\042\r\n",currentuser->name,param);
}
else
{
	sprintf(buf,"Somebody Says, \042%s\042\r\n",param);
}
	
sendmudmessagetoall(currentuser->room,buf);
return;
}

int whisper_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
char *param_notfirsttwotokens[255];
int count;

strcpy(param_notfirsttwotokens,command_tokens[2]);

for(count=3;count<tc;count++) {
	strcat(param_notfirsttwotokens,command_tokens[count]);
	strcat(param_notfirsttwotokens," ");
}

if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

sendmudmessage(currentuser,command_tokens[1],param_notfirsttwotokens);
return;
}

int pose_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
char *param[BUF_SIZE];
int count;

if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
return;
}

strcpy(param,command_tokens[1]);
strcat(param," ");

for(count=2;count<tc;count++) {
	strcat(param,command_tokens[count]);
	strcat(param," ");
}

pose(currentuser,param);
return;
}

int home_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
go(currentuser,currentuser->homeroom);
return;
}

int quit_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
quit(currentuser);
}

int version_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
char *buf[BUF_SIZE];

sprintf(buf,"%s %d.%d\r\n",MUD_NAME,MAJOR_VERSION,MINOR_VERSION);
send(currentuser->handle,buf,strlen(buf),0);
return;
}

int describe_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
char *param_notfirsttwotokens[255];
int count;

if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

strcpy(param_notfirsttwotokens,command_tokens[2]);

for(count=3;count<tc;count++) {
	strcat(param_notfirsttwotokens,command_tokens[count]);
	strcat(param_notfirsttwotokens," ");
}

describe(currentuser,command_tokens[1],param_notfirsttwotokens);
return; 
}

int get_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);  
	return;
}

pickup(currentuser,command_tokens[1]);
return;
}

int drop_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

drop(currentuser,command_tokens[1]);
return;
}

int help_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
showhelp(currentuser,command_tokens[1]);
return;
}

int password_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
changepassword(currentuser,command_tokens[1]);
return;
}

int spell_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

castspell(currentuser,command_tokens[1],command_tokens[2]);
return;
}

int fight_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
attack(currentuser,command_tokens[1]);
return;
}

int score_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
score(currentuser,command_tokens[1]);
return;
}

int inv_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
inventory(currentuser,command_tokens[1]);
return;
}

int give_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

give(currentuser,command_tokens[1],command_tokens[2]);
return;
}

int xyzzy_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
send(currentuser->handle,"Nothing happens\r\n",17,0);
return;
}

/* ********************************
*        Wizard commands       *
********************************
*/

int setrace_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(currentuser->status < WIZARD) {		/* can't do this yet */
	display_error(currentuser->handle,NOT_YET);
	return;
}

if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

updateuser(currentuser,command_tokens[1],"",0,0,"",0,0,0,0,command_tokens[1],"",0);
return;
}

/*
* set configuration options */

int set_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
char *buf[BUF_SIZE];
CONFIG config;

getconfigurationinformation(&config);

if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
display_error(currentuser->handle,NOT_YET);
return;
}

if(strcmp(command_tokens[1],"port") == 0) {	
	config.mudport=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"server") == 0) {	
	strcpy(config.mudserver,command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"object_reset_time") == 0) {	
	config.objectresettime=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"database_save_time") == 0) {	
	config.databaseresettime=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"user_reset_time") == 0) {	
	config.userresettime=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"database_save_time") == 0) {	
	config.databaseresettime=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}


if(strcmp(command_tokens[1],"config_save_time") == 0) {	
	config.configsavetime=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"allow_player_killing") == 0) {	
	if(strcmp(command_tokens[2],"true") == 0) config.allowplayerkilling=TRUE;
	if(strcmp(command_tokens[2],"false") == 0) config.allowplayerkilling=FALSE;

	updateconfigurationinformation(&config);

return;
}

if(strcmp(command_tokens[1],"allow_new_accounts_") == 0) {	
	if(strcmp(command_tokens[2],"true") == 0) config.allownewaccounts=TRUE;
	if(strcmp(command_tokens[2],"false") == 0) config.allownewaccounts=FALSE;

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"monster_reset_time") == 0) {	
	config.monsterresettime=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"ban_reset_time") == 0) {	
	config.banresettime=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"points_for_warrior") == 0) {	
	config.pointsforwarrior=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"points_for_hero") == 0) {	
	config.pointsforhero=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"points_for_warrior") == 0) {	
	config.pointsforwarrior=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"points_for_champion") == 0) {	
	config.pointsforchampion=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"points_for_superhero") == 0) {	
	config.pointsforsuperhero=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"points_for_enchanter") == 0) {	
	config.pointsforenchanter=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"points_for_sorceror") == 0) {	
	config.pointsforsorceror=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"points_for_necromancer") == 0) {	
	config.pointsfornecromancer=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"points_for_legend") == 0) {	
	config.pointsforlegend=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

if(strcmp(command_tokens[1],"points_for_wizard") == 0) {	
	config.pointsforwizard=atoi(command_tokens[2]);

	updateconfigurationinformation(&config);
	return;
}

sprintf(buf,"Bad option %s\r\n",command_tokens[2]);
send(currentuser->handle,buf,strlen(buf),0);
return;
}

int sethome_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

if(!*command_tokens[1] && currentuser->status < WIZARD) {			/* can't do this yet */
	display_error(currentuser->handle,NOT_YET);
	return;
}

updateuser(currentuser,command_tokens[0],"",atoi(command_tokens[1]),0,"",0,0,0,0,"","",0);
return;
}

int setgender_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

setgender(currentuser,command_tokens[1],command_tokens[2]);
return;
}

int setlevel_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

setlevel(currentuser,command_tokens[1],command_tokens[2]);
return;
}

int setclass_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {

if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

updateuser(currentuser,command_tokens[1],"",0,0,"",0,0,0,0,"",command_tokens[2],0);
return;
}

int setxp_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

setpoints(currentuser,command_tokens[1],command_tokens[2],EXPERIENCEPOINTS);
return;
}

int setmp_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

setpoints(currentuser,command_tokens[1],command_tokens[2],MAGICPOINTS);
return;
}

int setsp_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

	setpoints(currentuser,command_tokens[1],command_tokens[2],STAMINAPOINTS);
	return;
}

int banip_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

banip(currentuser,command_tokens[1]);
return;
}

int unban_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

unbanip(currentuser,command_tokens[1]);
return;
}

int ban_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
userban(currentuser,command_tokens[1]);
return;
}

int kill_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

return(killuser(currentuser,command_tokens[1]));
}

int create_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

createobject(currentuser,command_tokens[1]);
return;
}

int delete_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

deletething(currentuser,command_tokens[1]);
return;
}

int rename_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

renameobject(currentuser,command_tokens[1],command_tokens[2]);
return;
}

int chown_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {		/* set object owner */
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

setowner(currentuser,command_tokens[1],command_tokens[2]);
return;
}

int chmod_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

setobjectattributes(currentuser,command_tokens[1],command_tokens[2]);
return;
}

int copy_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

copyobject(currentuser,command_tokens[1],atoi(command_tokens[2]));
return;
}

int move_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

copyobject(currentuser,command_tokens[1],atoi(command_tokens[2]));
deletething(currentuser,command_tokens[1]);

return;
}

int dig_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
createroom(currentuser,command_tokens[1]);
return;
}

int force_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
char *param[BUF_SIZE];
int count;

if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

strcpy(param,command_tokens[1]);
strcat(param," ");

for(count=2;count<tc;count++) {
	strcat(param,command_tokens[count]);
	strcat(param," ");
}

force(currentuser,command_tokens[1],param);
return;
}

int listban_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
listbans(currentuser,command_tokens[1]);
return;
}

int go_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
display_error(currentuser->handle,NO_PARAMS);
return;
}

if(currentuser->status < WIZARD) {		/* can't do that */
	display_error(currentuser->handle,NOT_YET);  
	return;
}

go(currentuser,atoi(command_tokens[1]));
return;
}

int wall_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

wall(currentuser,command_tokens[1]);
return;
}

int take_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

take(currentuser,command_tokens[1],command_tokens[2]);
return;
}

int reload_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
	display_error(currentuser->handle,NOT_YET);  
	return;
}

getconfig();
}

int shutdown_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
mudshutdown(currentuser,command_tokens[1]);
return;
}

int addclass_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
class class;

if(tc < 2) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

strcpy(class.name,command_tokens[1]);
	
if(addnewclass(class) == -1) {
	display_error(currentuser->handle,NO_MEM);  
	return;
}

return;
}

int addrace_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
race race;

if(tc < 9) {
	display_error(currentuser->handle,NO_PARAMS);
	return;
}

strcpy(race.name,command_tokens[1]);
race.magic=atoi(command_tokens[2]);
race.strength=atoi(command_tokens[3]);
race.agility=atoi(command_tokens[4]);
race.luck=atoi(command_tokens[5]);
race.wisdom=atoi(command_tokens[6]);
race.intelligence=atoi(command_tokens[7]);
race.stamina=atoi(command_tokens[8]);

if(addnewrace(currentuser,&race) == -1) {
	display_error(currentuser->handle,NO_MEM);
	return;
}

return;
}

int dropdead_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
	updateuser(currentuser,currentuser->name,"",0,0,"",0,0,0,0,"","",0); 
	return;
}

int invisible_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
invisible(currentuser,command_tokens[1],FALSE);
return;
}

int visible_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
visible(currentuser,command_tokens[1],TRUE);
return;
}

int gag_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
gag(currentuser,command_tokens[1],TRUE);
return;
}

int ungag_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
gag(currentuser,command_tokens[1],FALSE);
return;
}

int setexit_statement(user *currentuser,int tc,char *command_tokens[BUF_SIZE][BUF_SIZE]) {
int room;
int r;
int whichroom;

if(strcmp(command_tokens[1],"here") == 0) {
	room=currentuser->room;
}
else
{
	room=atoi(command_tokens[1]);
}

if(strcmp(command_tokens[3],"here") == 0) {
	r=currentuser->room;
}
else
{
	r=atoi(command_tokens[3]);
}

for(whichroom=0;whichroom<11;whichroom++) {
	if(strcmp(roomnames[whichroom],command_tokens[4]) == 0) break;
}

setexit(currentuser,room,atoi(command_tokens[2]),whichroom);
return;
}

