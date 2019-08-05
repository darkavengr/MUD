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

#include "defs.h"

extern char *roomnames[11];

char *bs="Bad syntax\r\n";
char *idontunderstandthat="I don't understand that\r\n";
char *noparam="Missing parameters\r\n";
char *nomsg="Missing arguments";

int docommand(user *currentuser,char *s) {
 char *cb[10][BUF_SIZE];
 char *param[BUF_SIZE];
 char *buf[BUF_SIZE];
 char *param_notfirsttwotokens[255];
 int countx;
 int count;
 int x;
 int cpc;
 int r;
 char *b;
 room *currentroom;
 race race;
 class class;
 int whichroom;

 if(!*s) return;			/* no command */

 printf("commandroom=%lX\n",currentuser);

 currentroom=currentuser->roomptr;  

 memset(cb,0,10*BUF_SIZE);
 cpc=tokenize_line(s,cb," ");			/* tokenize line */

 strcpy(param_notfirsttwotokens,cb[2]);


 for(count=3;count<cpc;count++) {
  strcat(param_notfirsttwotokens,cb[count]);
  strcat(param_notfirsttwotokens," ");
 }


 strcpy(param,cb[2]);
 strcat(param," ");

 for(count=3;count<cpc;count++) {
  strcat(param,cb[count]);
  strcat(param," ");
 }
 
 printf("cb=%s\n",cb[0]);

 if(strcmp(cb[0],"north") == 0 || strcmp(cb[0],"n") == 0) {   /* go in directions (north south east west,etc) */
  go(currentuser,currentroom->exits[NORTH]);
  return;
 }

 if(strcmp(cb[0],"south") == 0 || strcmp(cb[0],"s") == 0) { 
  go(currentuser,currentroom->exits[SOUTH]);
  return;
 }

 if(strcmp(cb[0],"east") == 0 || strcmp(cb[0],"e") == 0) {   
  go(currentuser,currentroom->exits[EAST]);
  return;
 }

 if(strcmp(cb[0],"west") == 0 || strcmp(cb[0],"w") == 0) {   
  go(currentuser,currentroom->exits[WEST]);
  return;
 }

 if(strcmp(cb[0],"northeast") == 0 || strcmp(cb[0],"ne") == 0) {   
  go(currentuser,currentroom->exits[NORTHEAST]);
  return;
 }

 if(strcmp(cb[0],"northwest") == 0 || strcmp(cb[0],"nw") == 0) {   
  go(currentuser,currentroom->exits[NORTHWEST]);
  return;
 }

 if(strcmp(cb[0],"southeast") == 0 || strcmp(cb[0],"se") == 0) {   
  go(currentuser,currentroom->exits[SOUTHEAST]);
  return;
 }

 if(strcmp(cb[0],"southwest") == 0 || strcmp(cb[0],"sw") == 0) {   
  go(currentuser,currentroom->exits[SOUTHWEST]);
  return;
 }

 if(strcmp(cb[0],"up") == 0 || strcmp(cb[0],"u") == 0) {   
  go(currentuser,currentroom->exits[UP]);
  return;
 }

 if(strcmp(cb[0],"down") == 0 || strcmp(cb[0],"d") == 0) {   
  go(currentuser,currentroom->exits[DOWN]);
  return;
 }

 if(strcmp(cb[0],"look") == 0) {   			/* look at person or object */
  look(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"who") == 0) {   			/* get connected user/s */
  who(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"say") == 0) {   			/* say something */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  for(count=1;count<cpc;count++) {
   strcat(param,cb[count]);
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

 if(strcmp(cb[0],"whisper") == 0) {   			/* send private message */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  sendmudmessage(currentuser,cb[1],param_notfirsttwotokens);
  return;
 }

 if(strcmp(cb[0],":") == 0 || strcmp(cb[0],"pose") == 0) {  /* pose */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }
 
 strcpy(param,cb[1]);
 strcat(param," ");

 for(count=2;count<cpc;count++) {
  strcat(param,cb[count]);
  strcat(param," ");
 }
 
 pose(currentuser,param);
  return;
 }

 if(strcmp(cb[0],"home") == 0) {			/* go to home room */
  go(currentuser,currentuser->homeroom);
  return;
 }

 if(strcmp(cb[0],"quit") == 0) {			/* quit */
  quit(currentuser);
 }

 if(strcmp(cb[0],"version") == 0) {			/* display version */
  sprintf(buf,"%s %d.%d\r\n",MUD_NAME,MAJOR_VERSION,MINOR_VERSION);
  send(currentuser->handle,buf,strlen(buf),0);
  return;
 }

 if(strcmp(cb[0],"describe") == 0) {			/* describe youself or object or room */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  describe(currentuser,cb[1],param_notfirsttwotokens);
  return; 
 }

 if(strcmp(cb[0],"get") == 0) {				/* get object */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  pickup(currentuser,cb[1]);
  return;
}

 if(strcmp(cb[0],"drop") == 0) {			/* drop object */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  drop(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"help") == 0) {			/* get help */
  showhelp(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"password") == 0) {			/* change password */
  changepassword(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"spell") == 0) {			/* cast spell */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  castspell(currentuser,cb[1],cb[2]);
  return;
 }

 if(strcmp(cb[0],"f") == 0) {				/* fight monster or person */
  attack(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"score") == 0) {			/* displays score */
  score(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"inv") == 0) {				/* display inventory */
  inventory(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"give") == 0) {			/* give object to someone */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  give(currentuser,cb[1],cb[2]);
  return;
}

 if(strcmp(cb[0],"xyzzy") == 0) {                        /* ??? */
  send(currentuser->handle,"Nothing happens\r\n",17,0);
  return;
 }

/* ********************************
 *        Wizard commands       *
 ********************************
*/

if(strcmp(cb[0],"setrace") == 0) {
 if(currentuser->status < WIZARD) {		/* can't do this yet */
  send(currentuser->handle,notyet,strlen(notyet),0);
  return;
 }

  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

 updateuser(currentuser,cb[1],"",0,0,"",0,0,0,0,cb[1],"",0);
 return;
}

/*
 * set configuration options */

if(strcmp(cb[0],"set") == 0) {
  if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
   send(currentuser->handle,notyet,strlen(notyet),0);
   return;
  }

  if(strcmp(cb[1],"port") == 0) {	
   mudport=atoi(cb[2]);
   return;
  }

  if(strcmp(cb[1],"server") == 0) {	
   strcpy(mudserver,cb[2]);
   return;
  }

  if(strcmp(cb[1],"object_reset_time") == 0) {	
   objectresettime=atoi(cb[2]);
   return;
  }

  if(strcmp(cb[1],"database_save_time") == 0) {	
   databaseresettime=atoi(cb[2]);
   return;
  }

   if(strcmp(cb[1],"user_reset_time") == 0) {	
    userresettime=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"database_save_time") == 0) {	
    databaseresettime=atoi(cb[2]);
    return;
   }


   if(strcmp(cb[1],"config_save_time") == 0) {	
    configsavetime=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"allow_player_killing") == 0) {	
    if(strcmp(cb[2],"true") == 0) allowplayerkilling=TRUE;
    if(strcmp(cb[2],"false") == 0) allowplayerkilling=FALSE;
    return;
   }

   if(strcmp(cb[1],"allow_new_accounts_") == 0) {	
    if(strcmp(cb[2],"true") == 0) allownewaccounts=TRUE;
    if(strcmp(cb[2],"false") == 0) allownewaccounts=FALSE;
    return;
   }

   if(strcmp(cb[1],"monster_reset_time") == 0) {	
    monsterresettime=atoi(cb[2]);
    return;
   }


   if(strcmp(cb[1],"ban_reset_time") == 0) {	
    banresettime=atoi(cb[2]);
    return;
   }




   if(strcmp(cb[1],"points_for_warrior") == 0) {	
    pointsforwarrior=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"points_for_hero") == 0) {	
    pointsforhero=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"points_for_warrior") == 0) {	
    pointsforwarrior=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"points_for_champion") == 0) {	
    pointsforchampion=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"points_for_superhero") == 0) {	
    pointsforsuperhero=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"points_for_enchanter") == 0) {	
    pointsforenchanter=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"points_for_sorceror") == 0) {	
    pointsforsorceror=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"points_for_necromancer") == 0) {	
    pointsfornecromancer=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"points_for_legend") == 0) {	
    pointsforlegend=atoi(cb[2]);
    return;
   }

   if(strcmp(cb[1],"points_for_wizard") == 0) {	
    pointsforwizard=atoi(cb[2]);
    return;
   }

   sprintf(buf,"Bad option %s\r\n",cb[2]);
   send(currentuser->handle,buf,strlen(buf),0);
   return;
  }

 if(strcmp(cb[0],"setgender") == 0) {			/* set gender */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  setgender(currentuser,cb[1],cb[2]);
  return;
 }


 if(strcmp(cb[0],"sethome") == 0) {			/* set home */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

 if(!*cb[1] && currentuser->status < WIZARD) {			/* can't do this yet */
   send(currentuser->handle,notyet,strlen(notyet),0);
   return;
  }

 updateuser(currentuser,cb[0],"",atoi(cb[1]),0,"",0,0,0,0,"","",0);
 return;
 }

 if(strcmp(cb[0],"setgender") == 0) {			/* set gender */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  setgender(currentuser,cb[1],cb[2]);
  return;
 }

 if(strcmp(cb[0],"setlevel") == 0) {			/* set level */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  setlevel(currentuser,cb[1],cb[2]);
  return;
 }

 if(strcmp(cb[0],"setclass") == 0) {			/* set class */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  updateuser(currentuser,cb[1],"",0,0,"",0,0,0,0,"",cb[2],0);
  return;
 }

 if(strcmp(cb[0],"setxp") == 0) {			/* set experience  */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  setpoints(currentuser,cb[1],cb[2],EXPERIENCEPOINTS);
  return;
 }

 if(strcmp(cb[0],"setmp") == 0) {			/* set magic  */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  setpoints(currentuser,cb[1],cb[2],MAGICPOINTS);
  return;
 }

 if(strcmp(cb[0],"setsp") == 0) {			/* set experience  */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  setpoints(currentuser,cb[1],cb[2],STAMINAPOINTS);
  return;
 }

 if(strcmp(cb[0],"banip") == 0) {			/* ban  */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  banip(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"unban") == 0) {			/* unban  */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  unbanip(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"ban") == 0) {			/* ban  */
  userban(currentuser,cb[1]);
  return;
 }


 if(strcmp(cb[0],"kill") == 0) {			/* kill  */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  return(killuser(currentuser,cb[1]));
 }

 if(strcmp(cb[0],"create") == 0) {			/* create object */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  createobject(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"delete") == 0) {			/* delete object */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  deletething(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"rename") == 0) {			/* rename object */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  renameobject(currentuser,cb[1],cb[2]);
  return;
 }

 if(strcmp(cb[0],"chown") == 0) {			/* set object owner */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  setowner(currentuser,cb[1],cb[2]);
  return;
 }

 if(strcmp(cb[0],"chmod") == 0) {			/* set object attributes */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  setobjectattributes(currentuser,cb[1],cb[2]);
  return;
 }

 if(strcmp(cb[0],"copy") == 0) {			/* move object */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  copyobject(currentuser,cb[1],atoi(cb[2]));
  return;
 }

 if(strcmp(cb[0],"move") == 0) {			/* move object */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  copyobject(currentuser,cb[1],atoi(cb[2]));
  deletething(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"dig") == 0) {				/* create room */
  createroom(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"force") == 0) {			/* force user */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  printf("cmd=%s\n",param);

  force(currentuser,cb[1],param);
  return;
 }

 if(strcmp(cb[0],"listban") == 0) {			/* force user */
  listbans(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"go") == 0) {				/* go somewhere */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  if(currentuser->status < WIZARD) {		/* can't do that */
   send(currentuser->handle,notyet,strlen(notyet),0);
   return;
  }

  go(currentuser,atoi(cb[1]));
  return;
 }

 if(strcmp(cb[0],"wall") == 0) {			/*send message to all */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  wall(currentuser,cb[1]);
  return;
 }

 if(strcmp(cb[0],"take") == 0) {			/* take object */
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  take(currentuser,cb[1],cb[2]);
  return;
 }

 if(strcmp(cb[0],"reload") == 0) {

  if(currentuser->status < ARCHWIZARD) {		/* can't do this yet */
   send(currentuser->handle,notyet,strlen(notyet),0);
   return;
  }

  getconfig();
 }


 if(strcmp(cb[0],"shutdown") == 0) {
  mudshutdown(currentuser,cb[1]);
  return;
 }


 if(strcmp(cb[0],"addclass") == 0) {
  if(cpc < 2) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  strcpy(class.name,cb[1]);
  
  if(addnewclass(class) == -1) {
   send(currentuser->handle,mudnomem,strlen(mudnomem),0);
   return;
  }

  return;
 }

 if(strcmp(cb[0],"addrace") == 0) {
  if(cpc < 9) {
   send(currentuser->handle,noparam,strlen(noparam),0);
   return;
  }

  strcpy(race.name,cb[1]);
  race.magic=atoi(cb[2]);
  race.strength=atoi(cb[3]);
  race.agility=atoi(cb[4]);
  race.luck=atoi(cb[5]);
  race.wisdom=atoi(cb[6]);
  race.intelligence=atoi(cb[7]);
  race.stamina=atoi(cb[8]);

  if(addnewrace(currentuser,&race) == -1) {
   send(currentuser->handle,mudnomem,strlen(mudnomem),0);
   return;
  }

  return;
 }
 
 if(strcmp(cb[0],"dropdead") == 0) {
  updateuser(currentuser,currentuser->name,"",0,0,"",0,0,0,0,"","",0); 
  return;
 }

 if(strcmp(cb[0],"visible") == 0) {
  visible(currentuser,cb[1],FALSE);
  return;
 }

 if(strcmp(cb[0],"invisible") == 0) {
  visible(currentuser,cb[1],TRUE);
  return;
 }

 if(strcmp(cb[0],"gag") == 0) {
  gag(currentuser,cb[1],TRUE);
  return;
 }

 if(strcmp(cb[0],"ungag") == 0) {
  gag(currentuser,cb[1],FALSE);
  return;
 }

 if(strcmp(cb[0],"setexit") == 0) {
  if(strcmp(cb[1],"here") == 0) {
   count=currentuser->room;
  }
  else
  {
   count=atoi(cb[1]);
  }

  if(strcmp(cb[3],"here") == 0) {
   r=currentuser->room;
  }
  else
  {
   r=atoi(cb[3]);
  }

  for(whichroom=0;whichroom<11;whichroom++) {
   if(strcmp(roomnames[whichroom],cb[4]) == 0) break;
  }

  setexit(currentuser,count,atoi(cb[2]),whichroom);
  return;
 }

 send(currentuser->handle,idontunderstandthat,strlen(idontunderstandthat),0);		/* bad command */
 return;
}

