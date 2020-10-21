
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

char *errors[] = { "NO error\r\n","\r\nout of memory\r\n","\r\nObject not found\r\n","\r\nYou can't do that yet\r\n",\
		   "\r\nUsername is required\r\n","\r\nThat username already exists\r\n","\r\nGender must be either male or female\r\n",\
		   "\r\nUnknown player class\r\n","\r\nUnknown race\r\n","\r\nYou must enter a password\r\n","\r\nUser BANNED\r\n",\
		   "\r\nInvalid login\r\n","\r\nPlayer vs player combat not allowed\r\n","\r\nI don't understand that\r\n",\
		   "\r\nMissing parameters\r\n","\r\nMissing arguments\r\n","\r\nInvalid exit\r\n","\r\nCan't create objects here\r\n",\
		   "\r\nObject already exists\r\n","\r\nRoom not found\r\n","\r\nCan't create room\r\n", "Permission denied\r\n",\
		   "\r\nPassword is not strong enough\r\n","\r\nThat spell needs a higher level user to cast it\r\n",\
		   "\r\nYou don't have enough magic points to do that\r\n","\r\nYou can't put spells on users in haven rooms\r\n",\
		   "\r\nNo target for spell\r\n","\r\nSpell not found\r\n","\r\nUser already banned\r\n","\r\nUnknown user\r\n","\r\nInvalid level\r\n",\
		   "\r\nYou can't go that way\r\n","\r\nThis person cannot be killed\r\n","\r\nGAME OVER. You have 0 stamina points and are dead\r\n",\
		   "\r\nUnknown help topic\r\n","\r\nYou can't kill wizards" };

int display_error(int socket,int error) {
 send(socket,errors[error],strlen(errors[error]),0);
}

