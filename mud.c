
#ifdef __linux__
 #include <netdb.h>
 #include <sys/socket.h>
 #include <sys/types.h> 
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <sys/stat.h>
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
#include <sqlite3.h>
#include <stdbool.h>
#include "version.h"
#include "bool.h"
#include "state.h"
#include "class.h"
#include "race.h"
#include "errors.h"
#include "user.h"
#include "config.h"
#include "getconfig.h"
#include "password.h"
#include "monster.h"

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
char *RacesHeader="Name        Magic Strength Agility Dexterity Luck Wisdom Intelligence Stamina\r\n";
char *PasswordStrengthMessage="Enter a password.It must be at least %d characters long and include a lower-case letter, an uppercase letter, a number and a symbol.";

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
struct sockaddr_in clientip;
socklen_t clientiplen;
char *IPAddress[BUF_SIZE];
user *usernext;
char *OutputBuffer[1024];
char *bufptr;
struct timeval TimeoutValue;
time_t ObjectGenerateTime;
time_t ConfigResetTime;
time_t CurrentTime;
CONFIG config;
int CommandReturnValue;
sqlite3_stmt *SQLStatementHandle;
int returncode;

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
service.sin_port=htons(config.port);
						
if(bind(ListenSocket,&service,sizeof(service)) == -1) { 		/* bind to socket  */
	printf("mud: Unable to bind to socket\n");
	close(ListenSocket);
	exit(-1);
}

/* loop and accept connections */

printf("Waiting for connections on port %d\n",config.port);

if(listen(ListenSocket,MAX_BACKLOG) == -1) {			/* listen on socket  */
	printf("mud: Unable to listen on socket\n");
	exit(-1);
}

FD_ZERO(&currentset);

MaxSocket=ListenSocket;			/* maximum socket */

time(&ObjectGenerateTime);			/* update reset time */
ObjectGenerateTime += config.ObjectGenerateTime;

time(&ConfigResetTime);
ConfigResetTime += config.ConfigurationSaveTime;

GenerateObjects();		/* create new objects */
GenerateMonsters();	/* create monsters */

/*
 * The main event loop. This resets the object, saves the configuration information.
 * It then checks each connection in turn to see if there is data sent from the
 * user and processes it
 */

while(1) {
	 time(&CurrentTime);		/* get time */

	 if(CurrentTime > ObjectGenerateTime) {		/* update objects */
		 printf("mud: Generating objects\n");

		 GenerateObjects();		/* create new objects */

		time(&ObjectGenerateTime);			/* update reset time */
  		ObjectGenerateTime += config.ObjectGenerateTime;
 	}

	if(CurrentTime > ConfigResetTime) {		/* update config */

		if(GetConfigurationUpdatedFlag() == TRUE) {
			printf("mud: Updating configuration\n");

			UpdateConfigurationFile(config);
		}

		time(&ConfigResetTime);			/* update reset time */
		ConfigResetTime += config.ConfigurationSaveTime;
	}

	MoveMonster();		/* move a monster */

	/* check for data on sockets */

	FD_SET(ListenSocket,&currentset);		

	readset=currentset;
	TimeoutValue.tv_sec=5;			/* set timeout */

	/* wait until there is data ready to be read, or it times out */

//	printf("MaxSocket+1=%d\n",MaxSocket+1);

	retval=select(MaxSocket+1,&readset,NULL,NULL,&TimeoutValue);	
	if(retval == -1) {
		perror("mud select():");
//		exit(1);
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

	        		clientiplen=sizeof(struct sockaddr_in);			/* get IP7 address */
  	     			getpeername(AcceptSocket,(struct sockaddr*)&clientip,&clientiplen);
 
	     			strncpy(IPAddress,inet_ntoa(clientip.sin_addr),BUF_SIZE);
	
	     			if(CheckIfBanned(IPAddress) == TRUE) { /* check if banned */
					PrintError(SocketCount,USER_BANNED);

					FD_CLR(AcceptSocket,&currentset);
	        			close(AcceptSocket);
			        }
				
			        send(AcceptSocket,config.BannerMessage,strlen(config.BannerMessage),0);  	/* send banner message

				/* send username prompt */

		       		if(config.AllowNewAccounts == TRUE) {
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
				//printf("SocketCount=%d %d\n",SocketCount,connections[SocketCount].connectionstate);

	  			memset(connections[SocketCount].OutputBuffer,0,BUF_SIZE);

				/* get line from connection */		

				if((connections[SocketCount].connectionstate == STATE_CHECKLOGIN) || (connections[SocketCount].connectionstate == STATE_GETGENDER)) {	
					//printf("state=%d\n",connections[SocketCount].connectionstate);

					retval=recv(SocketCount,connections[SocketCount].OutputBuffer,1,0);
					if((retval == -1) || (retval == 0)) {
						close(SocketCount);

						FD_CLR(SocketCount,&currentset);
			  			break;
			 		}
					
					/* send backspace, then space, then another backspace to overwrite character */
					if(strlen(connections[SocketCount].OutputBuffer) > 0) {
						bufptr=connections[SocketCount].OutputBuffer;
						bufptr += strlen(connections[SocketCount].OutputBuffer) - 1;

						if(((char) *bufptr != '\r') && ((char) *bufptr != '\n')) {
							send(SocketCount,"\010",1,0);
							send(SocketCount," ",1,0);
							send(SocketCount,"\010",1,0);
						}

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

			       			strncpy(connections[SocketCount].upass,connections[SocketCount].buf,BUF_SIZE);
				       		break;

					case STATE_GETPASSWORD:			/* prompt for password */
						strncpy(connections[SocketCount].uname,connections[SocketCount].buf,BUF_SIZE);
	
						if(strncmp(connections[SocketCount].uname,"new",BUF_SIZE) == 0 && config.AllowNewAccounts == TRUE) {   /* create new account if allowed */
					       		send(SocketCount,NewUsernamePrompt,strlen(NewUsernamePrompt),0);
					       		connections[SocketCount].connectionstate=STATE_GETNEWPASS; 
						}
						else
						{
							send(SocketCount,PasswordPrompt,strlen(PasswordPrompt),0);
							connections[SocketCount].connectionstate=STATE_CHECKLOGIN;

							DisableOutput(SocketCount);		/* hide text input */
						}
	
						break;

					case STATE_CHECKLOGIN:			/* check username and password */
						EnableOutput(SocketCount);		/* show text input */

						strncpy(connections[SocketCount].upass,connections[SocketCount].buf,BUF_SIZE);

						if(LoginUser(SocketCount,connections[SocketCount].uname,connections[SocketCount].upass) == 0) {
							connections[SocketCount].connectionstate=STATE_GETCOMMAND;
						}	
						else
						{

							PrintError(SocketCount,INVALID_LOGIN);

					 		if(config.AllowNewAccounts == TRUE) {
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

						connections[SocketCount].user->socket=SocketCount;

						if(go(connections[SocketCount].user,connections[SocketCount].user->homeroom) == -1) {	/* go to room */
							PrintError(connections[SocketCount].user->socket,GetLastError(connections[SocketCount].user));
						}

						memset(connections[SocketCount].buf,0,BUF_SIZE);

					 	connections[SocketCount].connectionstate=STATE_GETCOMMAND;

						send(SocketCount,connections[SocketCount].user->username,strlen(connections[SocketCount].user->username),0);
						send(SocketCount,">",1,0);
						break;

						/* these states are for creating a new user */

					case STATE_GETNEWPASS:			/* get new password */
						if(CheckIfUserExists(connections[SocketCount].buf) == TRUE) {	/* username exists */
							PrintError(SocketCount,USERNAME_EXISTS);

							send(SocketCount,NewUsernamePrompt,strlen(NewUsernamePrompt),0);
							connections[SocketCount].connectionstate=STATE_GETNEWPASS; /* stay in state */	
						}
						else
						{
							strncpy(connections[SocketCount].uname,connections[SocketCount].buf,BUF_SIZE);

							sprintf(OutputBuffer,PasswordStrengthMessage,MINIMUM_PASSWORD_LENGTH);
							send(SocketCount,OutputBuffer,strlen(OutputBuffer),0);

							send(SocketCount,PasswordPrompt,strlen(PasswordPrompt),0);
							connections[SocketCount].connectionstate=STATE_GETGENDER; /* next state */
						}

						break;

					case STATE_GETGENDER:			/* get gender */
						if(!*connections[SocketCount].buf) {
							PrintError(SocketCount,NO_PASSWORD);

							send(SocketCount,PasswordPrompt,strlen(PasswordPrompt),0);

			         			connections[SocketCount].connectionstate=STATE_GETGENDER; /* loop state */
							break;
	                        		}

						if(CheckPasswordStrength(connections[SocketCount].buf) == FALSE) {	/* weak password */
							PrintError(currentuser,WEAK_PASSWORD);  
						
							connections[SocketCount].connectionstate=STATE_GETGENDER; /* loop state */
							break;
						}

						strncpy(connections[SocketCount].upass,connections[SocketCount].buf,BUF_SIZE);
		
	 			 		connections[SocketCount].connectionstate=STATE_GETDESC; /* next state */

						send(SocketCount,DescriptionPrompt,strlen(DescriptionPrompt),0);
						break;

					case STATE_GETDESC:				/* check gender and prompt get description */				
						if(strncmp(connections[SocketCount].buf,"male",BUF_SIZE) == 0) {
							connections[SocketCount].gender=MALE;
						}
						else if(strncmp(connections[SocketCount].buf,"female",BUF_SIZE) == 0) {
							connections[SocketCount].gender=FEMALE;
						}
				
						if((connections[SocketCount].gender != MALE)  && (connections[SocketCount].gender != FEMALE)) {
							PrintError(SocketCount,INVALID_GENDER);

							send(SocketCount,GenderPrompt,strlen(GenderPrompt),0);

							connections[SocketCount].connectionstate=STATE_GETDESC; /* stay on current */
							break;
						}

						strncpy(connections[SocketCount].description,connections[SocketCount].buf,BUF_SIZE);

						/* show list of races */

						send(SocketCount,ChoosePlayerRace,strlen(ChoosePlayerRace),0);

						if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM RACES;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	/* prepare SQL statement */
							SetLastError(currentuser,IO_ERROR);
							return(-1);
						}

						send(SocketCount,RacesHeader,strlen(RacesHeader),0);

						while(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {
								sprintf(OutputBuffer,"%10s %5d %7d %8d %8d %6d %4d %6d %12d\r\n",\
								     sqlite3_column_text(SQLStatementHandle,RACE_NAME_COLUMN),\	
								     sqlite3_column_int(SQLStatementHandle,RACE_MAGIC_COLUMN),\	
								     sqlite3_column_int(SQLStatementHandle,RACE_STRENGTH_COLUMN),\
								     sqlite3_column_int(SQLStatementHandle,RACE_AGILITY_COLUMN),\
								     sqlite3_column_int(SQLStatementHandle,RACE_DEXTERITY_COLUMN),\
								     sqlite3_column_int(SQLStatementHandle,RACE_LUCK_COLUMN),\
								     sqlite3_column_int(SQLStatementHandle,RACE_WISDOM_COLUMN),\
								     sqlite3_column_int(SQLStatementHandle,RACE_INTELLIGENCE_COLUMN),\
								     sqlite3_column_int(SQLStatementHandle,RACE_STAMINA_COLUMN));

							send(SocketCount,OutputBuffer,strlen(OutputBuffer),0);
						}

						
						sqlite3_finalize(SQLStatementHandle);

						send(SocketCount,RacePrompt,strlen(RacePrompt),0);
						connections[SocketCount].connectionstate=STATE_GETRACE; /* next state */
						break;

					case STATE_GETRACE:				/* get description and prompt for race */
						strncpy(connections[SocketCount].race,connections[SocketCount].buf,BUF_SIZE);
						
						/* show list of classes */

						send(SocketCount,ChoosePlayerClass,strlen(ChoosePlayerClass),0);
						
						if(sqlite3_prepare_v2(GetDatabaseHandle(),"SELECT * FROM CLASSES;",-1,&SQLStatementHandle,NULL) != SQLITE_OK) {	/* prepare SQL statement */
							return(-1);
						}

						while(sqlite3_step(SQLStatementHandle) == SQLITE_ROW) {
							sprintf(OutputBuffer,"%s\r\n",sqlite3_column_text(SQLStatementHandle,CLASS_NAME_COLUMN));	
				
							send(SocketCount,OutputBuffer,strlen(OutputBuffer),0);
						} 

						sqlite3_finalize(SQLStatementHandle);

						send(SocketCount,ClassPrompt,strlen(ClassPrompt),0);

						connections[SocketCount].connectionstate=STATE_GETCLASS; /* next state */
						break;
					
					case STATE_GETCLASS:
						strncpy(connections[SocketCount].class,connections[SocketCount].buf,BUF_SIZE);

						if(CreateUser(SocketCount,
							      connections[SocketCount].uname,
							      connections[SocketCount].upass,\
						              connections[SocketCount].description,\
							      DEFAULT_USER_LEVEL,\
							      connections[SocketCount].gender,\
							      DEFAULT_HOME_ROOM,\
							      DEFAULT_MAGIC_POINTS,\
							      DEFAULT_STAMINA_POINTS,\
							      DEFAULT_EXPERIENCE_POINTS,\
                                                              connections[SocketCount].race,\
							      connections[SocketCount].class,\
							      0) == -1) {	/* can't create account */

							sprintf(OutputBuffer,"Unable to create user %s.Connection terminated\r\n",connections[SocketCount].uname);
							send(SocketCount,OutputBuffer,strlen(OutputBuffer),0);

							FD_CLR(SocketCount,&readset);
							close(SocketCount);
					 	}

						if(LoginUser(SocketCount,connections[SocketCount].uname,connections[SocketCount].upass) == -1) {
							printf("mud: login error\n");
						}
		
						usernext=FindFirstUser();		/* find first user */

						while(usernext != NULL) {

							if(strncmp(usernext->username,connections[SocketCount].uname,BUF_SIZE) == 0) {
								connections[SocketCount].user=usernext;
								break;
							}

							usernext=FindNextUser(usernext);		/* find next user */
						}

						sprintf(OutputBuffer,"Created user %s\r\n",usernext->username);
						send(SocketCount,OutputBuffer,strlen(OutputBuffer),0);
			
						connections[SocketCount].user->socket=SocketCount;
						go(connections[SocketCount].user,usernext->homeroom);
					
						connections[SocketCount].connectionstate=STATE_GETCOMMAND;
					
					case STATE_PLAYAGAIN_PROMPT:
						if(((char) *connections[SocketCount].buf == 'y') || 
						   ((char) *connections[SocketCount].buf == 'Y')) {
							go(connections[SocketCount].user,connections[SocketCount].user->homeroom);

							connections[SocketCount].connectionstate=STATE_GETCOMMAND;
						}
						else if(((char) *connections[SocketCount].buf == 'n') || 
						   	((char) *connections[SocketCount].buf == 'N')) {
							
								/* disconnect user */
								DisconnectUser(connections[SocketCount].user,connections[SocketCount].user);
								break;
						}

					case STATE_GETCOMMAND:		/* processing command */
						CommandReturnValue=ExecuteCommand(connections[SocketCount].user,connections[SocketCount].buf);
			           		if(CommandReturnValue == -1) {		/* error */
							PrintError(connections[SocketCount].user->socket,GetLastError(connections[SocketCount].user));
						}
						else if(CommandReturnValue == -2) {		/* player killed */
							connections[SocketCount].connectionstate=STATE_PLAYAGAIN_PROMPT;
							break;
						}
						
						connections[SocketCount].connectionstate=STATE_GETCOMMAND;	/* loop in state STATE_GETCOMMAND */
	
						send(SocketCount,connections[SocketCount].user->username,strlen(connections[SocketCount].user->username),0);
						send(SocketCount,">",1,0);
					}
	
					memset(connections[SocketCount].buf,0,BUF_SIZE);   
				}
			}
		}
       	}

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

