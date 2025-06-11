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

struct {
user *user;
char *statement;
unsigned int (*call_command)(user *,int,void *);		/* function pointer */
} statements[] = { {  NULL,"NORTH",&north_command },\
		 {  NULL,"N",&north_command },\
		 {  NULL,"NORTHWEST",&northwest_command },\
		 {  NULL,"NW",&northwest_command },\
		 {  NULL,"SOUTH",&south_command },\
		 {  NULL,"S",&south_command },\
		 {  NULL,"SOUTHEAST",&southeast_command },\
		 {  NULL,"SE",&southeast_command },\
		 {  NULL,"EAST",&east_command },\
		 {  NULL,"E",&east_command },\
		 {  NULL,"WEST",&west_command },\
		 {  NULL,"W",&west_command },\
		 {  NULL,"SOUTHWEST",&southwest_command },\
		 {  NULL,"SW",&southwest_command },\
		 {  NULL,"UP",&up_command },\
		 {  NULL,"U",&up_command },\
		 {  NULL,"DOWN",&down_command },\
		 {  NULL,"D",&down_command },\
		 {  NULL,"LOOK",&look_command },\
		 {  NULL,"WHO",&who_command },\
		 {  NULL,"SAY",&say_command },\
		 {  NULL,"WHISPER",&whisper_command },\
		 {  NULL,":",&pose_command },\
		 {  NULL,"POSE",&pose_command },\
		 {  NULL,"HOME",&home_command },\
		 {  NULL,"QUIT",&quit_command },\
		 {  NULL,"VERSION",&version_command },\
		 {  NULL,"DESCRIBE",&describe_command },\
		 {  NULL,"GET",&get_command },\
		 {  NULL,"DropObject",&drop_command },\
		 {  NULL,"HELP",&help_command },\
		 {  NULL,"PASSWORD",&password_command },\
		 {  NULL,"SPELL",&spell_command },\
		 {  NULL,"F",&fight_command },\
		 {  NULL,"SCORE",&score_command },\
		 {  NULL,"INV",&inv_command },\
		 {  NULL,"GIVE",give_command },\
		 {  NULL,"XYZZY",&xyzzy_command },\
		 {  NULL,"SETRACE",&setrace_command },\
		 {  NULL,"SET",&set_command },\
		 {  NULL,"SETHOME",&sethome_command },\
		 {  NULL,"SETGENDER",&setgender_command },\
		 {  NULL,"SETLEVEL",&setlevel_command },\
		 {  NULL,"SETCLASSS",&setclass_command },\
		 {  NULL,"SETXP",&setxp_command },\
		 {  NULL,"SETMP",&setmp_command },\
		 {  NULL,"SETSP",&setsp_command },\
		 {  NULL,"BANIP",&ban_command },\
		 {  NULL,"UNBAN",&unban_command },\
		 {  NULL,"BAN",&ban_command },\
		 {  NULL,"KILL",&kill_command },\
		 {  NULL,"CREATE",&create_command },\
		 {  NULL,"DELETE",&delete_command },\
		 {  NULL,"RENAME",&rename_command },\
		 {  NULL,"CHOWN",&chown_command },\
		 {  NULL,"CHMOD",&chmod_command },\
		 {  NULL,"COPY",&copy_command },\
		 {  NULL,"MOVE",&move_command },\
		 {  NULL,"DIG",&dig_command },\
		 {  NULL,"FORCE",&force_command },\
		 {  NULL,"LISTBAN",&listban_command },\
		 {  NULL,"GO",&go_command },\
		 {  NULL,"WALL",&wall_command },\
		 {  NULL,"TAKE",&take_command },\
		 {  NULL,"RELOAD",&reload_command },\
		 {  NULL,"SHUTDOWN",&shutdown_command },\
		 {  NULL,"ADDCLASS",&addclass_command },\
		 {  NULL,"ADDRACE",&addrace_command },\
		 {  NULL,"DROPDEAD",&dropdead_command },\
		 {  NULL,"VISIBLE",&visible_command },\
		 {  NULL,"INVISIBLE",&invisible_command },\
		 {  NULL,"GAG",&gag_command },\
		 {  NULL,"UNGAG",&ungag_command },\
		 {  NULL,"SETEXIT",&setexit_command },\
	         { NULL,NULL } };

int ExecuteCommand(user *currentuser,char *command) {
char *CommandTokens[BUF_SIZE][BUF_SIZE];
int TokenCount;
int RoomLoop;
int StatementCount;

if(!*command) return(0);			/* no command */

printf("command=%command\n",command);

memset(CommandTokens,0,10*BUF_SIZE);
TokenCount=TokenizeLine(command,CommandTokens," ");			/* tokenize line */

StatementCount=0;

/* do statement */

do {
	if(statements[StatementCount].statement == NULL) break;

	ToUppercase(CommandTokens[0]);

	if(strcmp(statements[StatementCount].statement,CommandTokens[0]) == 0) return(statements[StatementCount].call_command(currentuser,TokenCount,CommandTokens));

	StatementCount++;

} while(statements[StatementCount].statement != NULL);

SetLastError(currentuser,BAD_COMMAND);
return(-1);
}

int north_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[NORTH]));
}

int south_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[SOUTH]));
}

int east_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[EAST]));
}

int west_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[NORTH]));
}

int northwest_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[NORTHWEST]));
}

int southwest_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[SOUTHWEST]));
}

int southeast_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[SOUTHEAST]));
}

int northeast_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[NORTHEAST]));
}

int up_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[UP]));
}

int down_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
room *currentroom=currentuser->roomptr;

return(go(currentuser,currentroom->exits[DOWN]));
}

int look_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {

return(look(currentuser,CommandTokens[1]));
}

int who_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {

return(who(currentuser,CommandTokens[1]));
}

int say_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int count;
char *param[BUF_SIZE];
char *buf[BUF_SIZE];

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

for(count=1;count<TokenCount;count++) {
	strcat(param,CommandTokens[count]);
	strcat(param," ");
}

if((currentuser->flags & USER_INVISIBLE) == 0) {
	sprintf(buf,"%command Says, \042%command\042\r\n",currentuser->name,param);
}
else
{
	sprintf(buf,"Somebody Says, \042%command\042\r\n",param);
}
	
return(SendMessageToAllInRoom(currentuser->room,buf));
}

int whisper_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *param_notfirsttwotokens[BUF_SIZE];
int count;

strcpy(param_notfirsttwotokens,CommandTokens[2]);

for(count=3;count<TokenCount;count++) {
	strcat(param_notfirsttwotokens,CommandTokens[count]);
	strcat(param_notfirsttwotokens," ");
}

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SendMessage(currentuser,CommandTokens[1],param_notfirsttwotokens));
}

int pose_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *param[BUF_SIZE];
int count;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

strcpy(param,CommandTokens[1]);
strcat(param," ");

for(count=2;count<TokenCount;count++) {
	strcat(param,CommandTokens[count]);
	strcat(param," ");
}

pose(currentuser,param);
return(0);
}

int home_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(go(currentuser,currentuser->homeroom));
}

int quit_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
quit(currentuser);
}

int version_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *OutputMessage[BUF_SIZE];

sprintf(OutputMessage,"%command %d.%d\r\n",MUD_NAME,MAJOR_VERSION,MINOR_VERSION);
send(currentuser->handle,OutputMessage,strlen(OutputMessage),0);
return(0);
}

int describe_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *param_notfirsttwotokens[BUF_SIZE];
int count;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

strcpy(param_notfirsttwotokens,CommandTokens[2]);

for(count=3;count<TokenCount;count++) {
	strcat(param_notfirsttwotokens,CommandTokens[count]);
	strcat(param_notfirsttwotokens," ");
}

return(describe(currentuser,CommandTokens[1],param_notfirsttwotokens));
}

int get_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);  
	return(-1);
}

return(PickUpObject(currentuser,CommandTokens[1]));
}

int drop_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(DropObject(currentuser,CommandTokens[1]));
}

int help_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(ShowHelp(currentuser,CommandTokens[1]));
}

int password_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(ChangePassword(currentuser,CommandTokens[1]));
}

int spell_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(CastSpell(currentuser,CommandTokens[1],CommandTokens[2]));
}

int fight_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(attack(currentuser,CommandTokens[1]));
}

int score_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(DisplayScore(currentuser,CommandTokens[1]));
}

int inv_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(DisplayInventory(currentuser,CommandTokens[1]));
}

int give_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(GiveObjectToUser(currentuser,CommandTokens[1],CommandTokens[2]));
}

int xyzzy_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
send(currentuser->handle,"Nothing happens\r\n",17,0);
return(0);
}

/* ********************************
*        Wizard commands       *
********************************
*/

int setrace_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(currentuser->status < WIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

UpdateUser(currentuser,CommandTokens[1],"",0,0,"",0,0,0,0,CommandTokens[1],"",0);
return(0);
}

/*
* set configuration options */

int set_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *buf[BUF_SIZE];
CONFIG config;

GetConfigurationInformation(&config);

if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(strcmp(CommandTokens[1],"port") == 0) {	
	config.mudport=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"server") == 0) {	
	strcpy(config.mudserver,CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"object_reset_time") == 0) {	
	config.objectresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"database_save_time") == 0) {	
	config.databaseresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"user_reset_time") == 0) {	
	config.userresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"database_save_time") == 0) {	
	config.databaseresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}


if(strcmp(CommandTokens[1],"config_save_time") == 0) {	
	config.configsavetime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"allow_player_killing") == 0) {	
	if(strcmp(CommandTokens[2],"true") == 0) config.allowplayerkilling=TRUE;
	if(strcmp(CommandTokens[2],"false") == 0) config.allowplayerkilling=FALSE;

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"allow_new_accounts_") == 0) {	
	if(strcmp(CommandTokens[2],"true") == 0) config.allownewaccounts=TRUE;
	if(strcmp(CommandTokens[2],"false") == 0) config.allownewaccounts=FALSE;

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"monster_reset_time") == 0) {	
	config.monsterresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"ban_reset_time") == 0) {	
	config.banresettime=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"points_for_warrior") == 0) {	
	config.pointsforwarrior=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"points_for_hero") == 0) {	
	config.pointsforhero=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"points_for_warrior") == 0) {	
	config.pointsforwarrior=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"points_for_champion") == 0) {	
	config.pointsforchampion=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"points_for_superhero") == 0) {	
	config.pointsforsuperhero=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"points_for_enchanter") == 0) {	
	config.pointsforenchanter=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"points_for_sorceror") == 0) {	
	config.pointsforsorceror=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"points_for_necromancer") == 0) {	
	config.pointsfornecromancer=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"points_for_legend") == 0) {	
	config.pointsforlegend=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

if(strcmp(CommandTokens[1],"points_for_wizard") == 0) {	
	config.pointsforwizard=atoi(CommandTokens[2]);

	return(UpdateConfigurationInformation(&config));
}

sprintf(buf,"Bad option %command\r\n",CommandTokens[2]);
send(currentuser->handle,buf,strlen(buf),0);
return(-1);
}

int sethome_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(!*CommandTokens[1] && currentuser->status < WIZARD) {			/* can't do this yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

return(UpdateUser(currentuser,CommandTokens[0],"",atoi(CommandTokens[1]),0,"",0,0,0,0,"","",0));
}

int setgender_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetUserGender(currentuser,CommandTokens[1],CommandTokens[2]));
}

int setlevel_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetUserLevel(currentuser,CommandTokens[1],CommandTokens[2]));
}

int setclass_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(UpdateUser(currentuser,CommandTokens[1],"",0,0,"",0,0,0,0,"",CommandTokens[2],0));
}

int setxp_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetUserPoints(currentuser,CommandTokens[1],CommandTokens[2],EXPERIENCEPOINTS));
}

int setmp_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetUserPoints(currentuser,CommandTokens[1],CommandTokens[2],MAGICPOINTS));
}

int setsp_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetUserPoints(currentuser,CommandTokens[1],CommandTokens[2],STAMINAPOINTS));
}

int BanUserByIPAddress_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(BanUserByIPAddress(currentuser,CommandTokens[1]));
}

int unban_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(UnBanUserByIPAddress(currentuser,CommandTokens[1]));
}

int ban_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(BanUserByName(currentuser,CommandTokens[1]));
}

int kill_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(KillUser(currentuser,CommandTokens[1]));
}

int create_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(CreateObject(currentuser,CommandTokens[1]));
}

int delete_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(DeleteObject(currentuser,CommandTokens[1]));
}

int rename_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(RenameObject(currentuser,CommandTokens[1],CommandTokens[2]));
}

int chown_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {		/* set object owner */
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetOwner(currentuser,CommandTokens[1],CommandTokens[2]));
}

int chmod_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(SetObjectAttributes(currentuser,CommandTokens[1],CommandTokens[2]));
}

int copy_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(CopyObject(currentuser,CommandTokens[1],atoi(CommandTokens[2])));
}

int move_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(CopyObject(currentuser,CommandTokens[1],atoi(CommandTokens[2])) == -1) return(-1);

if(DeleteObject(currentuser,CommandTokens[1]) == -1) return(-1);

return(0);
}

int dig_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(CreateRoom(currentuser,CommandTokens[1]));
}

int force_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
char *param[BUF_SIZE];
int count;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

strcpy(param,CommandTokens[1]);
strcat(param," ");

for(count=2;count<TokenCount;count++) {
	strcat(param,CommandTokens[count]);
	strcat(param," ");
}

return(ForceUser(currentuser,CommandTokens[1],param));
}

int listban_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(ListBans(currentuser,CommandTokens[1]));
}

int go_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

if(currentuser->status < WIZARD) {		/* can't do that */
	SetLastError(currentuser,NOT_YET);  
	return(-1);
}

return(go(currentuser,atoi(CommandTokens[1])));
}

int wall_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(wall(currentuser,CommandTokens[1]));
}

int take_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

return(TakeObject(currentuser,CommandTokens[1],CommandTokens[2]));
}

int reload_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
	SetLastError(currentuser,NOT_YET);  
	return(-1);
}

return(GetConfiguration());
}

int shutdown_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(ShutdownServer(currentuser,CommandTokens[1]));
}

int addclass_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
class class;

if(TokenCount < 2) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

strcpy(class.name,CommandTokens[1]);
	
if(AddNewClass(currentuser,&class) == -1) {
	SetLastError(currentuser,NO_MEM);  
	return(-1);
}

return(0);
}

int addrace_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
race race;

if(TokenCount < 9) {
	SetLastError(currentuser,NO_PARAMS);
	return(-1);
}

strcpy(race.name,CommandTokens[1]);
race.magic=atoi(CommandTokens[2]);
race.strength=atoi(CommandTokens[3]);
race.agility=atoi(CommandTokens[4]);
race.luck=atoi(CommandTokens[5]);
race.wisdom=atoi(CommandTokens[6]);
race.intelligence=atoi(CommandTokens[7]);
race.stamina=atoi(CommandTokens[8]);

if(AddNewRace(&race) == -1) {
	SetLastError(currentuser,NO_MEM);
	return(-1);
}

return(0);
}

int dropdead_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(UpdateUser(currentuser,currentuser->name,"",0,0,"",0,0,0,0,"","",0));
}

int invisible_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(SetVisibleMode(currentuser,CommandTokens[1],FALSE));
}

int visible_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(SetVisibleMode(currentuser,CommandTokens[1],TRUE));
}

int gag_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(GagUser(currentuser,CommandTokens[1],TRUE));
}

int ungag_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
return(GagUser(currentuser,CommandTokens[1],FALSE));
}

int setexit_command(user *currentuser,int TokenCount,char *CommandTokens[BUF_SIZE][BUF_SIZE]) {
int room;
int r;
int RoomLoop;

if(strcmp(CommandTokens[1],"here") == 0) {
	room=currentuser->room;
}
else
{
	room=atoi(CommandTokens[1]);
}

if(strcmp(CommandTokens[3],"here") == 0) {
	r=currentuser->room;
}
else
{
	r=atoi(CommandTokens[3]);
}

for(RoomLoop=0;RoomLoop<11;RoomLoop++) {
	if(strcmp(GetDirectionName(RoomLoop),CommandTokens[4]) == 0) break;
}

return(SetExit(currentuser,room,atoi(CommandTokens[2]),RoomLoop));
}

