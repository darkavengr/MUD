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

#include "defines.h"

int changepassword(user *currentuser,char *u) {
char *newpassprompt="Enter new password:";
char *userx[BUF_SIZE];
char *encryptedpassword[BUF_SIZE];
char *newpass[BUF_SIZE];
char *buf[BUF_SIZE];

 if(!*u) {
  strcpy(userx,currentuser->name);		/* use default user and password */
 }
 else
 {
  if(currentuser->status < WIZARD) {		/* can't set other password unless a wizard */
   display_error(currentuser->handle,NOT_YET);
   return;
  }

  strcpy(userx,u);		/* use default user and password */
 }

 send(currentuser->handle,newpassprompt,strlen(newpassprompt),0);		/* get new password */
 getpassword(currentuser->handle,newpass);

 if(checkpasswordstrength(newpass) == FALSE) {	/* weak password */
  display_error(currentuser->handle,WEAK_PASSWORD);  
  return;
 }

 strcpy(buf,userx);
 strcpy(encryptedpassword,crypt(newpass,buf));

 updateuser(currentuser,userx,encryptedpassword,0,0,"",0,currentuser->staminapoints,0,0,"","",0);
 return;
}

int checkpasswordstrength(char *pass) {
 if(strlen(pass)  < 10) return(FALSE);
 
 if(strpbrk(pass,"abcdefghijklmnopqrstuvwxyz") == NULL) return(FALSE);
 if(strpbrk(pass,"ABCDEFGHIJKLMNOPQRSTUVWXYZ") == NULL) return(FALSE);
 if(strpbrk(pass,"0123456789") == NULL) return(FALSE);
 if(strpbrk(pass,"!\"£$%^&*()_-+={}[]:;@'~#<>,.?/|\¬`") == NULL) return(FALSE);

 return(TRUE);
}



int getpassword(int msgsocket,char *buf) {
 char *temp[BUF_SIZE];
 int count;
 char *b;
 char *x;

 x=buf;			

 memset(temp,0,BUF_SIZE);

 while(1) {
  if(recv(msgsocket,temp,1,0) == -1) return(-1);	/* read data */

  b=temp;

  send(msgsocket,"\b \b",3,0);

   if(*b == '\n') {
    if(strlen(buf) > 0) *b=0;
    return(count);
   }
  
   *x++=*b++;
   count++;
  }

return;
}

