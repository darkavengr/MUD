/* generate password */

#include <stdio.h>
#include "size.h"
#include "hashpassword.h"

#define _XOPEN_SOURCE

int main(int argc,char **argv)
{
char *hashedpassword[BUF_SIZE];

if(argc < 3) {			/* no arguments */
	printf("Generate password for AdventureMUD\n\n");
	printf("genpass [username] [password]\n");
	exit(0);
}

if(HashPassword(argv[1],argv[2],hashedpassword) == -1) {	/* hash password */	
	perror("genpass");
	exit(1);
}

printf("%s\n",hashedpassword);

exit(0);
}

