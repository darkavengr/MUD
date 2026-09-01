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

#include <crypt.h>

#include "bool.h"
#include "errors.h"
#include "user.h"
#include "password.h"

int ChangePassword(user *currentuser,char *username,char *password) {
char *EncryptedPassword[BUF_SIZE];
char *CurrentUserName[BUF_SIZE];

if(!*username) {
	strncpy(CurrentUserName,currentuser->username,BUF_SIZE);		/* use default user and password */
}
else
{
	if(currentuser->userlevel < WIZARD) {		/* can't set other password unless a wizard */
		SetLastError(currentuser,ACCESS_DENIED);
		return(-1);
	}

	strncpy(CurrentUserName,username,BUF_SIZE);		/* use default user and password */
}

if(CheckPasswordStrength(password) == FALSE) {	/* weak password */
	SetLastError(currentuser,WEAK_PASSWORD);  
	return(-1);
}

strncpy(EncryptedPassword,crypt(password,CurrentUserName),BUF_SIZE);

return(UpdateUser(currentuser,username,
		  username,
		  EncryptedPassword,
		  currentuser->homeroom,
		  currentuser->userlevel,
		  currentuser->description,
		  currentuser->magicpoints,
		  currentuser->staminapoints,
		  currentuser->experiencepoints,
		  currentuser->gender,
		  &currentuser->race,
		  &currentuser->userclass,
		  currentuser->flags));
}

int CheckPasswordStrength(char *password) {
if(strlen(password)  < MINIMUM_PASSWORD_LENGTH) return(FALSE);

if(strpbrk(password,"abcdefghijklmnopqrstuvwxyz") == NULL) return(FALSE);
if(strpbrk(password,"ABCDEFGHIJKLMNOPQRSTUVWXYZ") == NULL) return(FALSE);
if(strpbrk(password,"0123456789") == NULL) return(FALSE);
if(strpbrk(password,"!\"£$%^&*()_-+={}[]:;@'~#<>,.?/|\¬`") == NULL) return(FALSE);

return(TRUE);
}

