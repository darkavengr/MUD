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

#include "defines.h"

char *shutdownmsg="WARNING: server shutdown\r\n";

extern user *users;

int mudshutdown(user *currentuser,char *shutdownmessage) {
 room *currentroom;
 user *usernext;

 currentroom=currentuser->roomptr;

 if(currentuser->status < ARCHWIZARD) {		/* not yet */
  display_error(currentuser->handle,NOT_YET);
  return;
 }

if(!*shutdownmessage) {			/* use default message */
 wall(currentuser,shutdownmsg);
}
else
{
 wall(currentuser,shutdownmessage);	/* send warning */
}

usernext=users;

while(usernext != NULL) {
 close(usernext->handle);		/* close tcp connections */
 usernext=usernext->next;
}

#ifdef _WIN32
 WSACleanup();			/* windoze needs wsacleanup */
#endif

exit(0);				/* terminate server */
return;
}

