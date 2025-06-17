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

/* display help */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

#include "help.h"
#include "errors.h"
#include "user.h"

int ShowHelp(user *currentuser,char *HelpTopic) {
char *ReadBuffer[BUF_SIZE];
FILE *handle;
char *HelpFile[BUF_SIZE];

if(!*HelpTopic) {		                      /* get help file */
	strcpy(HelpFile,"help/help.txt");
}
else
{
	sprintf(HelpFile,"help/%s.txt",HelpTopic);
}

handle=fopen(HelpFile,"rb");
if(!handle) {                 /* can't open file */
	SetLastError(currentuser,INVALID_HELP_TOPIC);  
	return(-1);
}

do {
	fgets(ReadBuffer,BUF_SIZE,handle);

	send(currentuser->handle,ReadBuffer,strlen(ReadBuffer),0);
} while(!feof(handle));

fclose(handle);
return(0);
}

