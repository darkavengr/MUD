#include "string.h"
#include <stdarg.h>
#include <stdint.h>

#include <time.h>
#include "defines.h"

#define SECONDS_IN_YEAR	 31557600 
#define SECONDS_IN_MONTH 2629800
#define SECONDS_IN_DAY	86400
#define SECONDS_IN_HOUR	3600
#define SECONDS_IN_MINUTE 60

unsigned int itoa(unsigned int n, char s[]);
void reverse(char s[]);
unsigned int touppercase(char *string);
unsigned int wildcard_rename(char *name,char *mask,char *out);
uint32_t tohex(uint32_t hex,char *buf);
int tokenize_line(char *linebuf,char *tokens[BUF_SIZE][BUF_SIZE],char *split);


 /* itoa:  convert n to characters in s */
unsigned int itoa(unsigned int n, char s[]) {
unsigned int i, sign;
 
//     if ((sign = n) < 0) n = -n;          /* make n positive */

     i = 0;

     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */

     s[i] = '\0';

     reverse(s);
 }

void reverse(char s[]) {
unsigned int c;
unsigned int i;
unsigned int j;

for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
 c = s[i];
 s[i] = s[j];
 s[j] = c;
 }
}

unsigned int wildcard(char *mask,char *filename) {
 unsigned int count;
 unsigned int countx;
 char *x;
 char *y;
 char *buf[BUF_SIZE];
 char *b;
 char *d;
 char *mmask[BUF_SIZE];
 char *nname[BUF_SIZE];

 strcpy(mmask,mask);
 strcpy(nname,filename);

 touppercase(mmask);					/* to upper case */
 touppercase(nname);

 memset(buf,0,BUF_SIZE);				/* clear buffer */
 
 x=mmask;
 y=nname;

 for(count=0;count<strlen(mmask);count++) {
  if(*x == '*') {					/* match multiple characters */
   b=buf;
   x++;
   
   while(*x != 0) {					/* match multiple characters */
    if(*x == '*' || *x == '?') break;
    *b++=*x++;
   }

   b=filename;
   d=buf;

   for(countx=0;countx<strlen(nname);countx++) {
    if(memcmp(b,d,strlen(buf)) == 0) return(0);
    b++;
   }

   return(-1);
  }

  if(*x++ != *y++)  return(-1);
 }


return(0);
}

unsigned int touppercase(char *string) {
char *s;

s=string;

while(*s != 0) {			/* until end */

 if(*s >= 'a' && *s <= 'z') *s=*s-32;			/* to uppercase */
 s++;

 }
}

unsigned int wildcard_rename(char *name,char *mask,char *out) {
unsigned int count;
char *m;
char *n;
char *o;
char c;
unsigned int countx;
char *newmask[BUF_SIZE];
char *mmask[BUF_SIZE];
char *nname[BUF_SIZE];

strcpy(mmask,mask);
strcpy(nname,name);

 touppercase(mmask);					/* convert to uppercase */
 touppercase(nname);

/*replace any * wild cards with ? */

n=newmask;
m=mask;

for(count=0;count<strlen(mmask);count++) {
 c=*m;						/* get character from mask */

 if(c == '*') {					/* match many characters */
  for(countx=count;countx<8-count;countx++) {
    *n++='?';    
   }

 }
 else
 {
  *n++=*m;
 }

m++;

}

n=nname;
m=newmask;
o=out;


for(count=0;count<strlen(newmask);count++) {
 c=*m;						/* get character from mask */

 if(c == '?') {					/* match single character */
  *o++=*n++;
 }
 else
 {
  *o++=*m;
 }

 m++;
}

return;
}			

uint32_t tohex(uint32_t hex,char *buf) {
unsigned int count;
uint32_t h;
uint32_t shiftamount;
uint32_t mask;

char *b;
char *hexbuf[] = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"};

mask=0xF0000000;
shiftamount=28;

b=buf;

for(count=1;count<9;count++) {
 h=((hex & mask) >> shiftamount);	/* shift nibbles to end */

 shiftamount=shiftamount-4;
 mask=mask >> 4;  
 strcpy(b++,hexbuf[h]);
}

 return(NULL);
}
int tokenize_line(char *linebuf,char *tokens[][BUF_SIZE],char *split) {
char *tok;
char *b[BUF_SIZE];
int count=0;

strcpy(b,linebuf);		/* get copy */

tok=strtok(b,split);		/* get token */

while(tok != NULL) {
 strcpy(tokens[count],tok);
 count++;

 tok=strtok(NULL,split);
}

return(count);
}


unsigned int regexp(char *filename,char *mask) {
 unsigned int count;
 unsigned int countx;
 char *x;
 char *y;
 char *buf[BUF_SIZE];
 char *b;
 char *d;
 char *mmask[BUF_SIZE];
 char *nname[BUF_SIZE];

 strcpy(mmask,mask);
 strcpy(nname,filename);

 toupper(mmask);					/* to upper case */
 toupper(nname);

 memset(buf,0,BUF_SIZE);				/* clear buffer */
 
 x=mmask;
 y=nname;

 for(count=0;count<strlen(mmask);count++) {
  if(*x == '*') {					/* match multiple characters */
   b=buf;
   x++;
   
   while(*x != 0) {					/* match multiple characters */
    if(*x == '*' || *x == '?') break;
    *b++=*x++;
   }

   b=filename;
   d=buf;

   for(countx=0;countx<strlen(nname);countx++) {
    if(memcmp(b,d,strlen(buf)) == 0) return(TRUE);
    b++;
   }

   return(FALSE);
  }

  if(*x++ != *y++)  return(FALSE);
 }


return(TRUE);
}

int tcp_getline(int socket,char *buf) {
 char *temp[BUF_SIZE];
 int count;
 char *b;
 char *x;

 x=buf;			/* find end */

 while(1) {
  if(recv(socket,temp,1,0) == -1) return(-1);	/* read data */
   b=temp;

   if(*b == '\n') {
    if(strlen(buf) > 0) *b=0;
    return(count);
   }
  
   *x++=*b++;
   count++;
  }

return;
}

int getvaluefromtimestring(char *str) {
 struct tm time;
 int val;
 char *p;
 char *start;
 char *buf[BUF_SIZE];
 int count=0;
 int total;
 int day=0;
 int month=0;
 int year=0;
 int hour=0;
 int minute=0;
 int second=0;

 p=str;
 start=p;

 while(*p++ != 0) {		/* until end */

  count++;

  if(*p == 'y' || *p == 'm' || *p == 'd' || *p == 'h' || *p == 'm' || *p == 's') {
   memcpy(buf,start,count);
   count=0;

   if(*p == 'y') year=atoi(buf);
   if(*p == 'm') month=atoi(buf);
   if(*p == 'd') day=atoi(buf);
   if(*p == 'h') hour=atoi(buf);
   if(*p == 'm') minute=atoi(buf);
   if(*p == 's') second=atoi(buf);

   memset(buf,0,BUF_SIZE);
  }
}

total=second;
total += (minute * SECONDS_IN_MINUTE);
total += (hour * SECONDS_IN_HOUR);
total += (day * SECONDS_IN_DAY);
total += (month * SECONDS_IN_MONTH);
total += (month * SECONDS_IN_YEAR);

return(total);
}

int createtimestring(int time,char *b) {
char *z[10];
char *buf[255];


  if(time >= SECONDS_IN_YEAR) {
   sprintf(buf,"%dy ",time / SECONDS_IN_YEAR);
   strcat(b,buf);
   time -= SECONDS_IN_YEAR/(time / SECONDS_IN_YEAR);
  }

  if(time >= SECONDS_IN_MONTH) {
   sprintf(buf,"%dm ",time / SECONDS_IN_MONTH);
   strcat(b,buf);
   time -= SECONDS_IN_MONTH/(time / SECONDS_IN_MONTH);
  }

  if(time > SECONDS_IN_DAY) {
   sprintf(buf,"%dd ",time / SECONDS_IN_DAY);
   strcat(b,buf);
   time -= SECONDS_IN_DAY/(time / SECONDS_IN_DAY);
  }

  if(time >= SECONDS_IN_HOUR) {
   sprintf(buf,"%dh ",time / SECONDS_IN_HOUR);
   strcat(b,buf);
   time -= SECONDS_IN_HOUR/(time / SECONDS_IN_HOUR);
  }

  if(time >= SECONDS_IN_MINUTE) {
   sprintf(buf,"%dm ",time / SECONDS_IN_MINUTE);
   strcat(b,buf);
   time -= SECONDS_IN_MINUTE/(time / SECONDS_IN_MINUTE);
  }

  if(time > SECONDS_IN_DAY) {
  sprintf(buf,"%ds ",time);
  strcat(b,buf);
 }
}

