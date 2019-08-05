
#ifdef __linux__
 #include <netdb.h>
 #include <sys/socket.h>
 #include <sys/types.h> 
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <sys/stat.h>
 #include <crypt.h>
#endif

#ifdef _WIN32
 #include <winsock2.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "defs.h"

#define  MAX_BACKLOG 14

char *banconf[BUF_SIZE];
ban *bans=NULL;

extern char *newpassprompt;

char *usernamerequired="Username is required\r\n";
char *userexists="That username already exists\r\n";
char *createpassprompt="Enter a password:";
char *genderprompt="Gender [enter 'male' or 'female']:";
char *badcreategender="Gender must be either male or female\r\n";
char *descprompt="Enter a description for yourself:";
char *chooserace="Choose a player race\r\n\r\n";
char *unknownclass="Unknown player class\r\n";
char *chooseclass="Choose a player class\r\n\r\n";
char *classprompt="Enter player class:";
char *badrace="Unknown race\r\n";
char *nopass="You must enter a password\r\n";
char *chooseplayerclass="Choose a player class\r\n";
char *chooseplayerrace="Choose a player race:\r\nName\t\Magic\t\Strength\t\Agility\tDexterity\tLuck\tWisdon\tIntelligence\tStamina\r\n";
char *raceprompt="Enter player race:";
char *newuserprompt="Enter new username:";
char *promptnewaccount="Enter username [type 'new' to create a new account]:";
char *userprompt="Enter username:";
char *banmsg="User BANNED\r\n";
char *passprompt="Enter password:";
char *loginbad="Invalid login\r\n";

struct {
 char *temp[BUF_SIZE];
 char *buf[BUF_SIZE];
 char *race[BUF_SIZE];
 char *uname[BUF_SIZE];
 char *upass[BUF_SIZE];
 char *description[BUF_SIZE];
 char *class[BUF_SIZE];
 int gender;
 int connectionstate;
 user *user;
} connections[1024];

int main(int argc,char *argv[]) {
int  as;
int  ls;
struct sockaddr_in service;
int size;
fd_set readset,currentset;
int maxsocket;
int retval;
int count;
char *b;
user *currentuser;
race *racenext;
class *classnext;
struct sockaddr_in clientip;
socklen_t clientiplen;
char *ipaddress[BUF_SIZE];
user *usernext;
char *temp[BUF_SIZE];
struct timeval tv;
time_t o,d,u,c,currenttime;

#ifdef WIN32
 WSADATA wsadata;
#endif

printf("AdventureMUD Version %d.%d\n",MAJOR_VERSION,MINOR_VERSION);

getconfig();				  /* get configuration */

#ifdef _WIN32				/* windows needs  wsastartup */
if(WSAStartup(MAKEWORD(2,2), &wsadata) != 0) {
  printf("mud: WSAStartup error\n");
 exit(-1);
}
#endif

ls=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);		/* create socket  */

if(ls == -1) {		
 printf("mud: Error creating socket \n");
 exit(-1);				 	
}

#ifdef _LINUX
 fcntl(ls,O_NONBLOCK);		/* make the socket nonblocking */
#endif

setsockopt(ls,SOL_SOCKET,SO_REUSEADDR,&(int){ 1 }, sizeof(int));		/* reuse socket */

memset(&service, 0, sizeof(service));

service.sin_addr.s_addr=htonl(htonl(INADDR_ANY));
service.sin_family=AF_INET;				
service.sin_port=htons(5000);
						
if(bind(ls,&service,sizeof(service)) == -1) { 		/* bind to socket  */
 printf("mud: Unable to bind to socket\n");
 close(ls);
 exit(-1);
}

/* loop and accept connections */

printf("Waiting for connections on %s port %d\n",mudserver,mudport);

if(listen(ls,MAX_BACKLOG) == -1) {			/* listen on socket  */
  printf("mud: Unable to listen on socket\n");
 exit(-1);
}

FD_ZERO(&currentset);
FD_SET(ls,&currentset);

maxsocket=ls;			/* maximum socket */

time(&o);			/* set times */
o=o+objectresettime;

time(&d);
d=d+databaseresettime;

time(&u);
u=u+userresettime;

time(&c);
c=c+configsavetime;

	
while(1) {
 time(&currenttime);

 if(currenttime > o) {		/* update objects */
  printf("mud: Updating objects\n");
  resetobjects();

  time(o);			/* set times */
  o=o+objectresettime;
 }

 if(currenttime > d) {		/* update database */
  printf("mud: Saving database\n");

  updatedatabase();
	
  time(&d);			/* set times */
  d=d+databaseresettime;
 }

 if(currenttime > users) {	/* update users */
  printf("mud: Saving users\n");

  updateusersfile();

  time(&u);			/* set times */
  u=u+userresettime;
 }

 if(currenttime > c) {		/* update config */
  printf("mud: Updating configuration\n");

  updateconfiguration();

  time(&c);			/* set times */
  c=c+configsavetime;
 }

 movemonster();

/* check for data on sockets */

 readset=currentset;
 tv.tv_sec=5;			/* set timeout */

 retval=select(maxsocket+1,&readset,NULL,NULL,&tv);


for(count=0;count <= maxsocket && retval > 0;count++) {		/* search sockets */
	
  if(FD_ISSET(count,&readset)) { 	/* ready to read */
   	   if(count == ls) {		/* new connection */
	     	as=accept(ls,(struct sockaddr*)NULL, NULL); 

		#ifdef _LINUX
 		 fcntl(as,O_NONBLOCK);		/* make the socket nonblocking */
		#endif

	     	FD_SET(as,&currentset);		/* add connection */
	        if(maxsocket < as) maxsocket=as;	/* new maximum */
				

	   	/* check if they're banned */

	        clientiplen=sizeof(struct sockaddr_in);			/* get ip address */
  	     	getpeername(as,(struct sockaddr*)&clientip,&clientiplen);
 
	     	strcpy(ipaddress,inet_ntoa(clientip.sin_addr));
	
	     	if(checkban(ipaddress) == TRUE) { /* check if banned */
	      	 send(as,banmsg,strlen(banmsg),0);
		 FD_CLR(as,&currentset);
	         close(as);
	        }

	        send(as,isbuf,issuecount,0);  	

       		if(allownewaccounts == TRUE) {
                      send(as,promptnewaccount,strlen(promptnewaccount),0);  	
		}
		else
		{
	               send(as,userprompt,strlen(userprompt),0);  	
		}

	   	       connections[as].connectionstate=1;

	   }

	 memset(connections[count].temp,0,BUF_SIZE);

        if(connections[count].connectionstate == 255) send(count,">",1,0);
	
         if(recv(count,connections[count].temp,BUF_SIZE,0) == -1) {	/* get data */
	  FD_CLR(count,&readset);
	  break;
	 }

	 strcat(connections[count].buf,connections[count].temp);

	 b=strpbrk(connections[count].buf,"\r");		/* if at end of line */
	 if(b != NULL) {
	  *b=0;


	 /* state machine for determing what to do for each step */

	   switch(connections[count].connectionstate) {

	     case STATE_GETUSER:			/* prompt for user name */
		if(allownewaccounts == TRUE) {
	               send(count,promptnewaccount,strlen(promptnewaccount),0);  	
		}
		else
		{
	               send(count,userprompt,strlen(userprompt),0);  	
		}

	   	       connections[count].connectionstate=STATE_GETPASSWORD;
		       strcpy(connections[count].upass,connections[count].buf);
		       break;

             case STATE_GETPASSWORD:			/* prompt for password */
			strcpy(connections[count].uname,connections[count].buf);

	               if(strcmp(connections[count].uname,"new") == 0 && allownewaccounts == TRUE) {   /* create new account if allowed */
 	      	 	 send(count,newuserprompt,strlen(newuserprompt),0);
	 		 connections[count].connectionstate=STATE_GETNEWPASS; 
			}
			else
			{
 	      	 	 send(count,passprompt,strlen(passprompt),0);
 			 connections[count].connectionstate=STATE_CHECKLOGIN; /* next */
			}

			break;



	    case STATE_CHECKLOGIN:			/* check username and password */	
			if(login(count,connections[count].uname,connections[count].uname,connections[count].upass) == 0) {
			 connections[count].connectionstate=STATE_GETCOMMAND;
			}
			else
			{
			 send(count,loginbad,strlen(loginbad),0);
			 connections[count].connectionstate=STATE_GETUSER;
			 break;
			}

 			usernext=users;

		        while(usernext != NULL) {		/* check username */
			 if(strcmp(usernext->name,connections[count].uname) == 0) {
			  connections[count].user=usernext;
  			  break;
			 }
                        
                         usernext=usernext->next;
			}

			/* send welcome message */

			sprintf(temp,"Welcome %s\r\n",usernext->name);
			send(count,temp,strlen(temp),0);

			memset(connections[count].buf,0,BUF_SIZE);

		 	connections[count].connectionstate=STATE_GETCOMMAND;
			break;

		/* these states are for creating a new user */

	    case STATE_GETNEWPASS:			/* get new password */
 			usernext=users;

		        while(usernext != NULL) {		/* check if name exists */
			 if(strcmp(usernext->name,connections[count].buf) == 0) {
			  send(count,userexists,strlen(userexists),0);
			  send(count,newuserprompt,strlen(newuserprompt),0);
	  	          connections[count].connectionstate=STATE_GETNEWPASS; /* stay in state */	
			  goto badbreak;
			 }
                        
                         usernext=usernext->next;
			}

			strcpy(connections[count].uname,connections[count].buf);
	 	      	send(count,newpassprompt,strlen(newpassprompt),0);
		        connections[count].connectionstate=STATE_GETGENDER; /* next state */

			badbreak:
			break;

	   case STATE_GETGENDER:			/* get gender */
			if(!*connections[count].buf) {
			 send(count,nopass,strlen(nopass),0);
    	      	         send(count,newpassprompt,strlen(newpassprompt),0);

		         connections[count].connectionstate=STATE_GETGENDER; /* loop state */
			 break;
                        }

			strcpy(connections[count].upass,connections[count].buf);
		
			send(count,genderprompt,strlen(genderprompt),0);
 			 connections[count].connectionstate=STATE_GETDESC; /* next state */
			break;

	  case	STATE_GETDESC:				/* check gender and prompt get description */				
			if(strcmp(connections[count].buf,"male") == 0) connections[count].gender=MALE;
			if(strcmp(connections[count].buf,"female") == 0) connections[count].gender=FEMALE;
			
			if(connections[count].gender != MALE  && connections[count].gender != FEMALE) {
			 send(count,badcreategender,strlen(badcreategender),0);
			 send(count,genderprompt,strlen(genderprompt),0);

			 connections[count].connectionstate=STATE_GETDESC; /* stay on current */
			 break;
			}

			send(count,descprompt,strlen(descprompt),0);
			connections[count].connectionstate=STATE_GETRACE; /* next state */
			break;

	  case STATE_GETRACE:				/* get description and prompt for race */
			strcpy(connections[count].description,connections[count].buf);
			send(count,chooseplayerrace,strlen(chooseplayerrace),0);

			racenext=races;				/* check if race exists */

			while(racenext != NULL) {
			 sprintf(temp,"%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\r\n",racenext->name,\	
			  racenext->magic,racenext->strength,racenext->agility,racenext->dexterity,\
			  racenext->luck,racenext->wisdom,racenext->intelligence,racenext->stamina);

			 send(count,temp,strlen(temp),0);
			 racenext=racenext->next;
			}
			
			send(count,raceprompt,strlen(raceprompt),0);
			connections[count].connectionstate=STATE_GETCLASS; /* next state */
			break;

 	  case STATE_GETCLASS:					/* get race and prompt for class */
			strcpy(connections[count].race,connections[count].buf);


			racenext=races;				/* check if race exists */

			while(racenext != NULL) {
	 		 touppercase(connections[count].buf);
	 		 touppercase(racenext->name);

			 if(strcmp(racenext->name,connections[count].buf) == 0) { 
			  connections[count].connectionstate=STATE_CREATEUSER; /* next state */
			  break;
			 }

			 racenext=racenext->next;
			}
	
			if(racenext == NULL) {
			 send(count,badrace,strlen(badrace),0);
	 		 send(count,raceprompt,strlen(raceprompt),0);
			 connections[count].connectionstate=STATE_GETCLASS; /* go to current state */
			 break;
			}

			send(count,chooseplayerclass,strlen(chooseplayerclass),0);

			classnext=classes;

			while(classnext != NULL) {
			  sprintf(temp,"%s\r\n",classnext->name);

  		          send(count,temp,strlen(temp),0);
			 classnext=classnext->next;
			}

			send(count,classprompt,strlen(classprompt),0);
			connections[count].connectionstate=STATE_CREATEUSER;			
			break;

	  case STATE_CREATEUSER:					/* check class and create user */
			strcpy(connections[count].class,connections[count].buf);

			classnext=classes;

			while(classnext != NULL) {
			 touppercase(connections[count].class);
	 		 touppercase(classnext->name);
	
			 if(strcmp(connections[count].class,classnext->name) == 0) break;
                             
			 classnext=classnext->next;
			}
		
			if(classnext == NULL) {		/* class not found, go back to state #14 */
			 connections[count].connectionstate=STATE_CREATEUSER;
			 send(count,unknownclass,strlen(unknownclass),0);
			 send(count,classprompt,strlen(classprompt),0);

			 break;
                        }

	
			if(createuser(count,connections[count].uname,connections[count].upass,\
			connections[count].gender,connections[count].description,connections[count].race,\
			connections[count].class) == -1) {	/* can't create account */
			  sprintf(temp,"Unable to create user (%s) connection terminated\r\n",strerror(errno));
			  send(count,temp,strlen(temp),0);
			  FD_CLR(count,&readset);
			  close(count);
			 }

			usernext=users;

		        while(usernext != NULL) {
			 if(strcmp(usernext->name,connections[count].uname) == 0) {
			  connections[count].user=usernext;
  			  break;
			 }
                        
                         usernext=usernext->next;
			}

			sprintf(temp,"Welcome %s\r\n",usernext->name);	/* send welcome message */
			send(count,temp,strlen(temp),0);

			go(connections[count].user,usernext->homeroom);	/* go to room */
			connections[count].connectionstate=STATE_GETCOMMAND;	/* next state */

			memset(connections[count].buf,0,BUF_SIZE);
			/* fall through */

	    case STATE_GETCOMMAND:
			docommand(connections[count].user,connections[count].buf);
			connections[count].connectionstate=STATE_GETCOMMAND;	/* loop in state STATE_GETCOMMAND */
		}
	
	      memset(connections[count].buf,0,BUF_SIZE);   
            }
         }
  	}
      }
}

