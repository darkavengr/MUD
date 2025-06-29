
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

char *GenderPrompt="Gender [enter 'male' or 'female']:";
char *DescriptionPrompt="Enter a description for yourself:";
char *ClassPrompt="Enter player class:";
char *ChoosePlayerClass="Choose a player class\r\n";
char *ChoosePlayerRace="Choose a player race:\r\nName\t\Magic\t\nStrength\t\nAgility\tDexterity\tLuck\tWisdon\tIntelligence\tStamina\r\n";
char *RacePrompt="Enter player race:";
char *NewUsernamePrompt="Enter new username:";
char *NewUserAccountPrompt="Enter username [type 'new' to create a new account]:";
char *UsernamePrompt="Enter username:";
char *PasswordPrompt="Enter password:";

struct {
	char *OutputBuffer[BUF_SIZE];
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
int AcceptSocket;
int ListenSocket;
struct sockaddr_in service;
int size;
int MaxSocket;
int retval;
int SocketCount;
user *currentuser;
race *RacePtr;
class *ClassPtr;
struct sockaddr_in clientip;
socklen_t clientiplen;
char *IPAddress[BUF_SIZE];
user *usernext;
char *OutputBuffer[BUF_SIZE];
struct timeval TimeoutValue;
time_t ObjectResetTime,DatabaseResetTime,UserResetTime,ConfigResetTime,currenttime;
CONFIG config;
int CommandReturnValue;

#ifdef WIN32
WSADATA wsadata;
#endif

printf("AdventureMUD Version %d.%d\n",MAJOR_VERSION,MINOR_VERSION);

GetConfiguration();				  /* get configuration */

GetConfigurationInformation(&config);

#ifdef _WIN32				/* Windows needs  WSAStartup */
if(WSAStartup(MAKEWORD(2,2), &wsadata) != 0) {
	printf("mud: WSAStartup error\n");
	exit(-1);
}
#endif

ListenSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);		/* create socket  */

if(ListenSocket == -1) {		
	printf("mud: Error creating socket \n");
	exit(-1);				 	
}

#ifdef _LINUX
	fcntl(ListenSocket,O_NONBLOCK);		/* make the socket nonblocking */
#endif

setsockopt(ListenSocket,SOL_SOCKET,SO_REUSEADDR,&(int){ 1 }, sizeof(int));		/* reuse socket */

memset(&service, 0, sizeof(service));

service.sin_addr.s_addr=htonl(htonl(INADDR_ANY));
service.sin_family=AF_INET;				
service.sin_port=htons(config.mudport);
						
if(bind(ListenSocket,&service,sizeof(service)) == -1) { 		/* bind to socket  */
	printf("mud: Unable to bind to socket\n");
	close(ListenSocket);
	exit(-1);
}

/* loop and accept connections */

printf("Waiting for connections on %s port %d\n",config.mudserver,config.mudport);

if(listen(ListenSocket,MAX_BACKLOG) == -1) {			/* listen on socket  */
	printf("mud: Unable to listen on socket\n");
	exit(-1);
}

FD_ZERO(&currentset);

MaxSocket=ListenSocket;			/* maximum socket */

time(&ObjectResetTime);			/* update reset time */
ObjectResetTime += config.objectresettime;

time(&DatabaseResetTime);
DatabaseResetTime += config.databaseresettime;

time(&UserResetTime);
UserResetTime += config.userresettime;

time(&ConfigResetTime);
ConfigResetTime += config.configsavetime;

GenerateObjects();		/* create new objects */
GenerateMonsters();	/* create monsters */
	
/*
 * The main event loop. This resets the object, saves the configuration information.
 * It then checks each connection in turn to see if there is data sent from the
 * user and processes it
 */

while(1) {
	 time(&currenttime);		/* get time */

	 if(currenttime > ObjectResetTime) {		/* update objects */
		// printf("mud: Updating objects\n");

		  GenerateObjects();		/* create new objects */

		time(&ObjectResetTime);			/* update reset time */
  		ObjectResetTime += config.objectresettime;
 	}

	 if(currenttime > DatabaseResetTime) {		/* update database */
  	 //	printf("mud: Saving database\n");

		UpdateDatabase();
	
		time(&DatabaseResetTime);			/* update reset time */
		DatabaseResetTime += config.databaseresettime;
	 }

	 if(currenttime > UserResetTime) {	/* update users */
	//	printf("mud: Saving users\n");

  		UpdateUsersFile();

  		time(&UserResetTime);			/* update reset time */
		UserResetTime += config.userresettime;
 	}

	if(currenttime > ConfigResetTime) {		/* update config */
	//	printf("mud: Updating configuration\n");

		updateconfiguration(config);

		time(&ConfigResetTime);			/* update reset time */
		ConfigResetTime += config.configsavetime;
	}

	MoveMonster();		/* move a monster */

	/* check for data on sockets */

	FD_SET(ListenSocket,&currentset);		

	readset=currentset;
	TimeoutValue.tv_sec=5;			/* set timeout */

	/* wait until there is data ready to be read, or it times out */

	retval=select(MaxSocket+1,&readset,NULL,NULL,&TimeoutValue);	
	if(retval == -1) {
		perror("mud:");
		exit(1);
	}

	for(SocketCount=0;SocketCount <= MaxSocket && retval > 0;++SocketCount) {		/* search sockets */
	
		if(FD_ISSET(SocketCount,&readset)) { 	/* there is data ready to read */

			if(SocketCount == ListenSocket) {		/* new connection */
				AcceptSocket=accept(ListenSocket,(struct sockaddr*)NULL, NULL); 

				#ifdef _LINUX
 					fcntl(AcceptSocket,O_NONBLOCK);		/* make the socket nonblocking */
				#endif
	
	     			FD_SET(AcceptSocket,&currentset);		/* add connection */
	
	        		if(MaxSocket < AcceptSocket) MaxSocket=AcceptSocket;	/* new maximum */
				
	   			/* check if the user is banned */

	        		clientiplen=sizeof(struct sockaddr_in);			/* get ip address */
  	     			getpeername(AcceptSocket,(struct sockaddr*)&clientip,&clientiplen);
 
	     			strcpy(IPAddress,inet_ntoa(clientip.sin_addr));
	
	     			if(CheckIfBanned(IPAddress) == TRUE) { /* check if banned */
					PrintError(currentuser->handle,USER_BANNED);

					FD_CLR(AcceptSocket,&currentset);
	        			close(AcceptSocket);
			        }

			        send(AcceptSocket,config.isbuf,config.issuecount,0);  	/* send banner message

				/* send username prompt */

		       		if(config.allownewaccounts == TRUE) {
                      			send(AcceptSocket,NewUserAccountPrompt,strlen(NewUserAccountPrompt),0);  	
				}
				else
				{
			               send(AcceptSocket,UsernamePrompt,strlen(UsernamePrompt),0);  	
				}

				connections[AcceptSocket].connectionstate=STATE_GETPASSWORD;
	 		}
	 		else
         		{				/* existing connection */

	  			memset(connections[SocketCount].OutputBuffer,0,BUF_SIZE);

				/* get line from connection */		

				if((connections[SocketCount].connectionstate == STATE_CHECKLOGIN) || (connections[SocketCount].connectionstate == STATE_GETNEWPASS)) {	
					retval=recv(SocketCount,connections[SocketCount].OutputBuffer,1,0);
					if(retval == -1) {
						FD_CLR(SocketCount,&currentset);
			  			break;
			 		}
					
					/* send backspace, then space, then another backspace to overwrite character */
					if(strlen(connections[SocketCount].OutputBuffer) > 0) {
						send(SocketCount,"\010",1,0);
						send(SocketCount," ",1,0);
						send(SocketCount,"\010",1,0);

						strcat(connections[SocketCount].buf,connections[SocketCount].OutputBuffer);	/* add to buffer */
					}

				}
				else
				{
					retval=recv(SocketCount,connections[SocketCount].OutputBuffer,BUF_SIZE,0);
					if(retval == -1) {	/* get data */
						FD_CLR(SocketCount,&currentset);
			  			break;
			 		}

					if(strlen(connections[SocketCount].OutputBuffer) > 0) {
						strcat(connections[SocketCount].buf,connections[SocketCount].OutputBuffer);	/* add to buffer */
					}
				}

				if(strpbrk(connections[SocketCount].buf,"\n") == NULL) continue;	/* no newline found */

				RemoveNewLine(connections[SocketCount].buf);	/* remove newline character */
	
				memset(connections[SocketCount].OutputBuffer,0,BUF_SIZE);

				 /* state machine to determine what to do for each step */
	
				switch(connections[SocketCount].connectionstate) {
					
	     	 			case STATE_GETUSER:			/* get username */
	   		       			connections[SocketCount].connectionstate=STATE_GETPASSWORD;

			       			strcpy(connections[SocketCount].upass,connections[SocketCount].buf);
				       		break;

					case STATE_GETPASSWORD:			/* prompt for password */
						strcpy(connections[SocketCount].uname,connections[SocketCount].buf);
	
						if(strcmp(connections[SocketCount].uname,"new") == 0 && config.allownewaccounts == TRUE) {   /* create new account if allowed */
					       		send(SocketCount,NewUsernamePrompt,strlen(NewUsernamePrompt),0);
					       		connections[SocketCount].connectionstate=STATE_GETNEWPASS; 
						}
						else
						{
							send(SocketCount,PasswordPrompt,strlen(PasswordPrompt),0);
							connections[SocketCount].connectionstate=STATE_CHECKLOGIN;

							DisableOutput(SocketCount);		/* hide text input */
							printf("waiting for password\n");
						}
	
						break;

					case STATE_CHECKLOGIN:			/* check username and password */
						EnableOutput(SocketCount);		/* show text input */

						strcpy(connections[SocketCount].upass,connections[SocketCount].buf);

						if(LoginUser(SocketCount,connections[SocketCount].uname,connections[SocketCount].upass) == 0) {
							connections[SocketCount].connectionstate=STATE_GETCOMMAND;
						}	
						else
						{

							PrintError(SocketCount,INVALID_LOGIN);

					 		if(config.allownewaccounts == TRUE) {
		        	              			send(SocketCount,NewUserAccountPrompt,strlen(NewUserAccountPrompt),0);  	
							}
							else
							{
						               send(SocketCount,UsernamePrompt,strlen(UsernamePrompt),0);  	
							}

							connections[AcceptSocket].connectionstate=STATE_GETPASSWORD;	/* move to next state */
							break;
						}

						usernext=GetUserPointerByName(connections[SocketCount].uname); /* find user */
						if(usernext != NULL) connections[SocketCount].user=usernext;
			
						/* send welcome message */

						sprintf(OutputBuffer,"Welcome %s\r\n",connections[SocketCount].uname);
						send(SocketCount,OutputBuffer,strlen(OutputBuffer),0);

						connections[SocketCount].user->loggedin=TRUE;
						connections[SocketCount].user->handle=SocketCount;

						if(go(connections[SocketCount].user,connections[SocketCount].user->homeroom) == -1) {	/* go to room */
							PrintError(connections[SocketCount].user->handle,GetLastError(connections[SocketCount].user));
						}

						memset(connections[SocketCount].buf,0,BUF_SIZE);

					 	connections[SocketCount].connectionstate=STATE_GETCOMMAND;
						send(SocketCount,">",1,0);
						break;

						/* these states are for creating a new user */

					case STATE_GETNEWPASS:			/* get new password */
						usernext=FindFirstUser();		/* find first user */

						while(usernext != NULL) {

							if(strcmp(usernext->name,connections[SocketCount].buf) == 0) {
								PrintError(currentuser->handle,USERNAME_EXISTS);

								send(SocketCount,NewUsernamePrompt,strlen(NewUsernamePrompt),0);
								connections[SocketCount].connectionstate=STATE_GETNEWPASS; /* stay in state */	

								goto badbreak;
				 			}

							usernext=FindNextUser(usernext);		/* find next user */
						}

						strcpy(connections[SocketCount].uname,connections[SocketCount].buf);

						send(SocketCount,PasswordPrompt,strlen(PasswordPrompt),0);
						connections[SocketCount].connectionstate=STATE_GETGENDER; /* next state */

						badbreak:
						break;

					case STATE_GETGENDER:			/* get gender */
						if(!*connections[SocketCount].buf) {
							PrintError(currentuser->handle,NO_PASSWORD);

							send(SocketCount,PasswordPrompt,strlen(PasswordPrompt),0);

			         			connections[SocketCount].connectionstate=STATE_GETGENDER; /* loop state */
							break;
	                        		}

						strcpy(connections[SocketCount].upass,connections[SocketCount].buf);
		
						send(SocketCount,GenderPrompt,strlen(GenderPrompt),0);
	 			 		connections[SocketCount].connectionstate=STATE_GETDESC; /* next state */
						break;

					case STATE_GETDESC:				/* check gender and prompt get description */				
						if(strcmp(connections[SocketCount].buf,"male") == 0) connections[SocketCount].gender=MALE;
						if(strcmp(connections[SocketCount].buf,"female") == 0) connections[SocketCount].gender=FEMALE;
				
						if(connections[SocketCount].gender != MALE  && connections[SocketCount].gender != FEMALE) {
							PrintError(currentuser->handle,BAD_GENDER);

							send(SocketCount,GenderPrompt,strlen(GenderPrompt),0);

							connections[SocketCount].connectionstate=STATE_GETDESC; /* stay on current */
							break;
						}

						send(SocketCount,DescriptionPrompt,strlen(DescriptionPrompt),0);
						connections[SocketCount].connectionstate=STATE_GETRACE; /* next state */
						break;

					case STATE_GETRACE:				/* get description and prompt for race */
						strcpy(connections[SocketCount].description,connections[SocketCount].buf);
						send(SocketCount,ChoosePlayerRace,strlen(ChoosePlayerRace),0);

						/* show list of races */

						RacePtr=FindFirstRace();

						while(RacePtr != NULL) {
							sprintf(OutputBuffer,"%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\r\n",RacePtr->name,\	
							RacePtr->magic,RacePtr->strength,RacePtr->agility,RacePtr->dexterity,\
							RacePtr->luck,RacePtr->wisdom,RacePtr->intelligence,RacePtr->stamina);

							send(SocketCount,OutputBuffer,strlen(OutputBuffer),0);
							
							RacePtr=FindNextRace(RacePtr);
						}
			
						send(SocketCount,RacePrompt,strlen(RacePrompt),0);
						connections[SocketCount].connectionstate=STATE_GETCLASS; /* next state */
						break;

					case STATE_GETCLASS:					/* get race and prompt for class */
						strcpy(connections[SocketCount].class,connections[SocketCount].buf);
	
						ClassPtr=FindFirstClass();				/* check if class exists */

						while(ClassPtr != NULL) {
							ToUppercase(connections[SocketCount].buf);
							ToUppercase(ClassPtr->name);

							if(strcmp(ClassPtr->name,connections[SocketCount].buf) == 0) { 
								connections[SocketCount].connectionstate=STATE_CREATEUSER; /* next state */
					  			break;
					 		}

					 		ClassPtr=FindNextClass(ClassPtr);
						}
	
						if(ClassPtr == NULL) {
							PrintError(currentuser->handle,BAD_CLASS);
	
							send(SocketCount,ClassPrompt,strlen(ClassPrompt),0);
							connections[SocketCount].connectionstate=STATE_GETCLASS; /* go to current state */
							break;
						}
	
						send(SocketCount,ChoosePlayerClass,strlen(ChoosePlayerClass),0);

						ClassPtr=FindFirstClass();

						while(ClassPtr != NULL) {
							sprintf(OutputBuffer,"%s\r\n",ClassPtr->name);
							send(SocketCount,OutputBuffer,strlen(OutputBuffer),0);
	
							ClassPtr=FindNextClass(ClassPtr);	
						}

						send(SocketCount,ClassPrompt,strlen(ClassPrompt),0);
						connections[SocketCount].connectionstate=STATE_CREATEUSER;			
						break;

					case STATE_CREATEUSER:					/* check class and create user */
						strcpy(connections[SocketCount].class,connections[SocketCount].buf);

						if(CreateUser(SocketCount,connections[SocketCount].uname,connections[SocketCount].upass,\
							connections[SocketCount].gender,connections[SocketCount].description,connections[SocketCount].race,\
							connections[SocketCount].class) == -1) {	/* can't create account */

							sprintf(OutputBuffer,"Unable to create user (%s) connection terminated\r\n",strerror(errno));
							send(SocketCount,OutputBuffer,strlen(OutputBuffer),0);

							FD_CLR(SocketCount,&readset);
							close(SocketCount);
					 	}

						usernext=FindFirstUser();		/* find first user */

						while(usernext != NULL) {

							if(strcmp(usernext->name,connections[SocketCount].uname) == 0) {
								connections[SocketCount].user=usernext;
								break;
							}

							usernext=FindNextUser(usernext);		/* find next user */
						}

						connections[SocketCount].connectionstate=STATE_CHECKLOGIN;
						break;

					case STATE_PLAYAGAIN_PROMPT:
						if(((char) *connections[SocketCount].buf == 'y') || 
						   ((char) *connections[SocketCount].buf == 'Y')) {
							go(connections[SocketCount].user,connections[SocketCount].user->homeroom);

							connections[SocketCount].connectionstate=STATE_GETCOMMAND;
						}
						else if(((char) *connections[SocketCount].buf == 'n') || 
						   	((char) *connections[SocketCount].buf == 'N')) {
								connections[SocketCount].user->loggedin=FALSE; /* mark as logged out */

								DisconnectUser(connections[SocketCount].user);		/* disconnect user */
						}

					case STATE_GETCOMMAND:		/* processing command */
						CommandReturnValue=ExecuteCommand(connections[SocketCount].user,connections[SocketCount].buf);
			           		if(CommandReturnValue == -1) {
							PrintError(connections[SocketCount].user->handle,GetLastError(connections[SocketCount].user));
						}
						else if(CommandReturnValue == -2) {
							connections[SocketCount].connectionstate=STATE_PLAYAGAIN_PROMPT;
							break;
						}
	
						connections[SocketCount].connectionstate=STATE_GETCOMMAND;	/* loop in state STATE_GETCOMMAND */

						send(SocketCount,">",1,0);
					}
	
					memset(connections[SocketCount].buf,0,BUF_SIZE);   
				}
			}
		}
       	}

}

void DisconnectUser(user *currentuser) {
FD_CLR(currentuser->handle,&currentset);
close(currentuser->handle);
}

void DisableOutput(int socket) {
char *response[BUF_SIZE];

send(socket,"\377",1,0);				/* disable local echo */
send(socket,"\373",1,0);
send(socket,"\001",1,0);

recv(socket,response,3,0);				/* get response */			
}

void EnableOutput(int socket) {
char *response[BUF_SIZE];

send(socket,"\377",1,0);				/* enable local echo */
send(socket,"\374",1,0);
send(socket,"\001",1,0);

recv(socket,response,3,0);				/* get response */			
}

