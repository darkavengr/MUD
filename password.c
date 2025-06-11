/*
* change password
*/

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

#include "bool.h"
#include "errors.h"
#include "user.h"

int ChangePassword(user *currentuser,char *username,char *password) {
char *EncryptedPassword[BUF_SIZE];
char *CurrentUserName[BUF_SIZE];

if(!*username) {
	strcpy(CurrentUserName,currentuser->name);		/* use default user and password */
}
else
{
	if(currentuser->status < WIZARD) {		/* can't set other password unless a wizard */
		SetLastError(currentuser,NOT_YET);
		return(-1);
	}

	strcpy(CurrentUserName,username);		/* use default user and password */
}

if(CheckPasswordStrength(password) == FALSE) {	/* weak password */
	SetLastError(currentuser,WEAK_PASSWORD);  
	return(-1);
}

strcpy(EncryptedPassword,crypt(password,CurrentUserName));

return(UpdateUser(currentuser,CurrentUserName,EncryptedPassword,0,0,"",0,currentuser->staminapoints,0,0,"","",0));
}

int CheckPasswordStrength(char *password) {
if(strlen(password)  < 10) return(FALSE);

if(strpbrk(password,"abcdefghijklmnopqrstuvwxyz") == NULL) return(FALSE);
if(strpbrk(password,"ABCDEFGHIJKLMNOPQRSTUVWXYZ") == NULL) return(FALSE);
if(strpbrk(password,"0123456789") == NULL) return(FALSE);
if(strpbrk(password,"!\"£$%^&*()_-+={}[]:;@'~#<>,.?/|\¬`") == NULL) return(FALSE);

return(TRUE);
}



void getpassword(int msgsocket,char *buf) {
char passchar;
char *bufptr=buf;

while(1) {
	if(recv(msgsocket,*&passchar,1,0) == -1) return(-1);	/* read data */

	send(msgsocket,"\b \b",3,0);

	 if(passchar == '\n') {
	 	*bufptr=0;
	 	return;
	 }
	
	 *bufptr++=passchar;
	}

return(0);
}

