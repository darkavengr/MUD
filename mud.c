
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

#include "version.h"
#include "bool.h"
#include "state.h"
#include "class.h"
#include "race.h"
#include "errors.h"
#include "user.h"
#include "config.h"

#define  MAX_BACKLOG 14

extern race *races;
extern class *classes;

char *newpassprompt="Enter a password:";
char *genderprompt="Gender [enter 'male' or 'female']:";
char *descprompt="Enter a description for yourself:";
char *chooserace="Choose a player race\r\n\r\n";
char *chooseclass="Choose a player class\r\n\r\n";
char *classprompt="Enter player class:";
char *chooseplayerclass="Choose a player class\r\n";
char *chooseplayerrace="Choose a player race:\r\nName\t\Magic\t\nStrength\t\nAgility\tDexterity\tLuck\tWisdon\tIntelligence\tStamina\r\n";
char *raceprompt="Enter player race:";
char *newuserprompt="Enter new username:";
char *promptnewaccount="Enter username [type 'new' to create a new account]:";
char *userprompt="Enter username:";
char *passprompt="Enter password:";

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

fd_set readset,currentset;

int main(int argc,char *argv[]) {
int as;
int ls;
struct sockaddr_in service;
int size;
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
CONFIG config;

#ifdef WIN32
WSADATA wsadata;
#endif

printf("AdventureMUD Version %d.%d\n",MAJOR_VERSION,MINOR_VERSION);

getconfig();				  /* get configuration */

getconfigurationinformation(&config);

#ifdef _WIN32				/* Windows needs  WSAStartup */
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
service.sin_port=htons(config.mudport);
						
if(bind(ls,&service,sizeof(service)) == -1) { 		/* bind to socket  */
	printf("mud: Unable to bind to socket\n");
	close(ls);
	exit(-1);
}

/* loop and accept connections */

printf("Waiting for connections on %s port %d\n",config.mudserver,config.mudport);

if(listen(ls,MAX_BACKLOG) == -1) {			/* listen on socket  */
	printf("mud: Unable to listen on socket\n");
	exit(-1);
}

FD_ZERO(&currentset);

maxsocket=ls;			/* maximum socket */

time(&o);			/* update reset time */
o=o+config.objectresettime;

time(&d);
d=d+config.databaseresettime;

time(&u);
u=u+config.userresettime;

time(&c);
c=c+config.configsavetime;


resetobjects();		/* create new objects */
CreateMonster();	/* create monsters */
	
/*
 * The main event loop, this resets the object, saves the configuration information
 * It then checks each connection in turn to see if there is data sent from the
 * user and processes it if there is
 */

while(1) {
/* check if mud needs updating */
 
	 time(&currenttime);		/* get time */

	 if(currenttime > o) {		/* update objects */
		 printf("mud: Updating objects\n");

		  resetobjects();		/* create new objects */

		time(&o);			/* update reset time */
  		o=o+config.objectresettime;
 	}

	 if(currenttime > d) {		/* update database */
  	 	printf("mud: Saving database\n");

		updatedatabase();
	
		time(&d);			/* update reset time */
		d=d+config.databaseresettime;
	 }

	 if(currenttime > u) {	/* update users */
		printf("mud: Saving users\n");

  		updateusersfile();

  		time(&u);			/* update reset time */
		u=u+config.userresettime;
 	}

	if(currenttime > c) {		/* update config */
		printf("mud: Updating configuration\n");

		updateconfiguration(config);

		time(&c);			/* update reset time */
		c=c+config.configsavetime;
	}

	MoveMonster();		/* move a monster */

	/* check for data on sockets */

	FD_SET(ls,&currentset);		

	readset=currentset;
	tv.tv_sec=5;			/* set timeout */

	/* wait until there is data ready to be read, or it times out */

	retval=select(maxsocket+1,&readset,NULL,NULL,&tv);	
	if(retval == -1) {
		perror("mud:");
		exit(1);
	}

	for(count=0;count <= maxsocket && retval > 0;++count) {		/* search sockets */
	
		if(FD_ISSET(count,&readset)) { 	/* there is data ready to read */

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
					PrintError(currentuser->handle,USER_BANNED);

					FD_CLR(as,&currentset);
	        			close(as);
			        }

				/* send username prompt */
			        send(as,config.isbuf,config.issuecount,0);  	

		       		if(config.allownewaccounts == TRUE) {
                      			send(as,promptnewaccount,strlen(promptnewaccount),0);  	
				}
				else
				{
			               send(as,userprompt,strlen(userprompt),0);  	
				}

				connections[as].connectionstate=STATE_GETPASSWORD;
	 		}
	 		else
         		{				/* existing connection */
	  			memset(connections[count].temp,0,BUF_SIZE);

				/* get line from connection */
		
			        if(recv(count,connections[count].temp,BUF_SIZE,0) == -1) {	/* get data */
					FD_CLR(count,&currentset);
	  				break;
	 			}

				strcat(connections[count].buf,connections[count].temp);	/* add to buffer */

				removenewline(connections[count].buf);		/* remove newline character */

				 /* state machine for determing what to do for each step */
	
				switch(connections[count].connectionstate) {
					
	     	 			case STATE_GETUSER:			/* prompt for user name */
						if(config.allownewaccounts == TRUE) {
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
	
						if(strcmp(connections[count].uname,"new") == 0 && config.allownewaccounts == TRUE) {   /* create new account if allowed */
					       		send(count,newuserprompt,strlen(newuserprompt),0);
					       		connections[count].connectionstate=STATE_GETNEWPASS; 
						}
						else
						{
							send(count,passprompt,strlen(passprompt),0);
							connections[count].connectionstate=STATE_CHECKLOGIN; /* next */

							printf("continuing to login\n");
						}
	
						break;

					case STATE_CHECKLOGIN:			/* check username and password */	
						strcpy(connections[count].upass,connections[count].buf);

						if(login(count,connections[count].uname,connections[count].upass) == 0) {
							connections[count].connectionstate=STATE_GETCOMMAND;
						}	
						else
						{
							PrintError(count,INVALID_LOGIN);

							connections[count].connectionstate=STATE_GETUSER;
							break;
						}

						usernext=GetUserPointerByName(connections[count].uname); /* find user */
						if(usernext != NULL) connections[count].user=usernext;
			
						/* send welcome message */

						sprintf(temp,"Welcome %s\r\n",usernext->name);
						send(count,temp,strlen(temp),0);

						usernext->handle=count;
						usernext->loggedin=TRUE;

						if(go(usernext,usernext->homeroom) == -1) {	/* go to room */
							PrintError(usernext->handle,GetLastError(connections[count].user));
						}

						memset(connections[count].buf,0,BUF_SIZE);

					 	connections[count].connectionstate=STATE_GETCOMMAND;
						send(count,">",1,0);
						break;

						/* these states are for creating a new user */

					case STATE_GETNEWPASS:			/* get new password */
						usernext=FindFirstUser();		/* find first user */

						while(usernext != NULL) {

							if(strcmp(usernext->name,connections[count].buf) == 0) {
								PrintError(currentuser->handle,USERNAME_EXISTS);

								send(count,newuserprompt,strlen(newuserprompt),0);
								connections[count].connectionstate=STATE_GETNEWPASS; /* stay in state */	

								goto badbreak;
				 			}

							usernext=FindNextUser(usernext);		/* find next user */
						}

						strcpy(connections[count].uname,connections[count].buf);

						send(count,newpassprompt,strlen(newpassprompt),0);
						connections[count].connectionstate=STATE_GETGENDER; /* next state */

						badbreak:
						break;

					case STATE_GETGENDER:			/* get gender */
						if(!*connections[count].buf) {
							PrintError(currentuser->handle,NO_PASSWORD);

							send(count,newpassprompt,strlen(newpassprompt),0);

			         			connections[count].connectionstate=STATE_GETGENDER; /* loop state */
							break;
	                        		}

						strcpy(connections[count].upass,connections[count].buf);
		
						send(count,genderprompt,strlen(genderprompt),0);
	 			 		connections[count].connectionstate=STATE_GETDESC; /* next state */
						break;

					case STATE_GETDESC:				/* check gender and prompt get description */				
						if(strcmp(connections[count].buf,"male") == 0) connections[count].gender=MALE;
						if(strcmp(connections[count].buf,"female") == 0) connections[count].gender=FEMALE;
				
						if(connections[count].gender != MALE  && connections[count].gender != FEMALE) {
							PrintError(currentuser->handle,BAD_GENDER);

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

						/* show list of races */

						racenext=FindFirstRace();

						while(racenext != NULL) {
							sprintf(temp,"%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\r\n",racenext->name,\	
							racenext->magic,racenext->strength,racenext->agility,racenext->dexterity,\
							racenext->luck,racenext->wisdom,racenext->intelligence,racenext->stamina);

							send(count,temp,strlen(temp),0);
							
							racenext=FindNextRace(racenext);
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
							PrintError(currentuser->handle,BAD_RACE);
	
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

							PrintError(count,BAD_CLASS);
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

						usernext=FindFirstUser();		/* find first user */

						while(usernext != NULL) {

							if(strcmp(usernext->name,connections[count].uname) == 0) {
								connections[count].user=usernext;
								break;
							}

							usernext=FindNextUser(usernext);		/* find next user */
						}

						connections[count].connectionstate=STATE_CHECKLOGIN;
						break;

					case STATE_GETCOMMAND:		/* processing command */
			           		if(docommand(usernext,connections[count].buf) == -1) {
							PrintError(usernext->handle,GetLastError(connections[count].user));
						}

						connections[count].connectionstate=STATE_GETCOMMAND;	/* loop in state STATE_GETCOMMAND */

						send(count,">",1,0);
					}
	
					memset(connections[count].buf,0,BUF_SIZE);   
				}
			}
		}
       	}

}

void DisconnectUser(user *currentuser) {
FD_CLR(currentuser->handle,&currentset);
close(currentuser->handle);
}

