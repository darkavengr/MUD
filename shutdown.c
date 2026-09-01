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

#include "shutdown.h"
#include "errors.h"
#include "user.h"

char *DefaultShutdownMessage="WARNING: Server is shutting down NOW!\r\n";

int ShutdownServer(user *currentuser,char *shutdownmessage) {
room *currentroom;

currentroom=currentuser->roomptr;

if(currentuser->userlevel < DUNGEONMASTER) {		/* not yet */
	SetLastError(currentuser,ACCESS_DENIED);
	return(-1);
}

if(!*shutdownmessage) {			/* use default message */
	wall(currentuser,DefaultShutdownMessage);
}
else
{
	wall(currentuser,shutdownmessage);	/* send warning */
}

DisconnectUser(currentuser,"*");		/* disconnect all users */

CloseDatabase();		/* close database */

#ifdef _WIN32
WSACleanup();			/* Windoze needs WSACleanup() */
#endif

exit(0);				/* terminate server */
}

