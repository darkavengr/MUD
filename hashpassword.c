#include <stdio.h>
#include <unistd.h>
#include <crypt.h>
#include "size.h"

int HashPassword(char *salt,char *password,char *out) {
char *temp[BUF_SIZE];
char *outpass;

/* Hash password */

snprintf(temp,BUF_SIZE,"$6$%s",salt);		/* generate salt */

outpass=crypt(password,temp);	/* hash password */
if(outpass == NULL) return(-1);

outpass += (4 + strlen(salt));	/* skip over id bytes and salt */
strncpy(out,outpass,BUF_SIZE);

return(0);
}

