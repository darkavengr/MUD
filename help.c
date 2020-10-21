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

#include "defines.h"

char *helpprompt="-- Press any key to continue -- or type q to quit";

int showhelp(user *currentuser,char *ht) {
char *buf[BUF_SIZE];
char *b;
int count=0;
FILE *handle;

getcwd(buf,BUF_SIZE);				/* get directory */

if(!*ht) {		                      /* get help file */
 strcat(buf,"/help/help.txt");
}
else
{
 strcat(buf,"/help/");
 strcat(buf,ht);
 strcat(buf,".txt");
}

handle=fopen(buf,"rb");
if(handle == NULL) {                 /* can't open file */
 display_error(currentuser->handle,NO_HELP_TOPIC);  
 free(buf);
 return;
}

while(!feof(handle)) {                /* display text until end of file */
 fgets(buf,BUF_SIZE,handle);
 send(currentuser->handle,buf,strlen(buf),0);
}

fclose(handle);
return;
}
