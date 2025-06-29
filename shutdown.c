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

char *shutdownmsg="WARNING: server shutdown\r\n";

int ShutdownServer(user *currentuser,char *shutdownmessage) {
room *currentroom;

currentroom=currentuser->roomptr;

if(currentuser->status < ARCHWIZARD) {		/* not yet */
	SetLastError(currentuser,NOT_YET);
	return(-1);
}

if(!*shutdownmessage) {			/* use default message */
	wall(currentuser,shutdownmsg);
}
else
{
	wall(currentuser,shutdownmessage);	/* send warning */
}

DisconnectAllUsers();		/* disconnect all user */

#ifdef _WIN32
WSACleanup();			/* windoze needs wsacleanup */
#endif

exit(0);				/* terminate server */
}

