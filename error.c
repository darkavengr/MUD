
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

#include <string.h>

#include "defines.h"

char *errors[] = { "NO error\r\n","out of memory\r\n","Object not found\r\n","You can't do that yet\r\n",\
		   "Username is required\r\n","That username already exists\r\n","Gender must be either male or female\r\n",\
		   "Unknown player class\r\n","Unknown race\r\n","You must enter a password\r\n","User BANNED\r\n",\
		   "Invalid login\r\n","Player vs player combat not allowed\r\n","I don't understand that\r\n",\
		   "Missing parameters\r\n","Missing arguments\r\n","Invalid exit\r\n","Can't create objects here\r\n",\
		   "Object already exists\r\n","Room not found\r\n","Can't create room\r\n", "Permission denied\r\n",\
		    "Password is not strong enough\r\n","That spell needs a higher level user to cast it\r\n",\
		   "You don't have enough magic points to do that\r\n","You can't put spells on users in haven rooms\r\n",\
		   "No target for spell\r\n","Spell not found\r\n","User already banned\r\n","Unknown user\r\n","Invalid level\r\n",\
		   "You can't go that way\r\n","This person cannot be killed\r\n","GAME OVER. You have 0 stamina points and are dead\r\n",\
		   "Unknown help topic\r\n","You can't kill wizards" };	

int display_error(int socket,int error) {
	send(socket,errors[error],strlen(errors[error]),0);
}

