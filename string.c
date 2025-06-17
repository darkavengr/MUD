#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "bool.h"
#include "size.h"
#include "string.h"

#define SECONDS_IN_YEAR		31557600 
#define SECONDS_IN_MONTH 2629800
#define SECONDS_IN_DAY	86400
#define SECONDS_IN_HOUR	3600
#define SECONDS_IN_MINUTE 60

/* itoa:  convert n to characters in s */
void itoa(unsigned int n, char s[]) {
unsigned int i, sign;

//     if ((sign = n) < 0) n = -n;          /* make n positive */

i=0;

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

return;
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

ToUppercase(mmask);					/* to upper case */
ToUppercase(nname);

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

void ToUppercase(char *string) {
char *s;

s=string;

while(*s != 0) {			/* until end */
	if(*s >= 'a' && *s <= 'z') *s=*s-32;			/* to uppercase */
	s++;
}

return;
}

/*
 * Replace string using wildcard
 *
 * In: char *name	Input string
	   char *mask	Wildcard to match
	   char *out	Output string
 *
 * Returns nothing
 *
 */
void WildcardRename(char *name,char *mask,char *out) {
int count;
char *MaskPtr;
char *NewMaskPtr;
char *OutPtr;
int countx;
char *newmask[BUF_SIZE];
char *mmask[BUF_SIZE];
char *nname[BUF_SIZE];

ToUppercase(mask);					/* convert to uppercase */
ToUppercase(name);

/*replace any * wild cards with ? */

NewMaskPtr=newmask;
MaskPtr=mask;

for(count=0;count<strlen(mmask);count++) {
	if((char) *MaskPtr == '*') {					/* match many characters */
		for(countx=count;countx<8-count;countx++) {
			*NewMaskPtr++='?';	
		}

	}
	else
	{
		*NewMaskPtr++=*MaskPtr;
	}

	MaskPtr++;
}

NewMaskPtr=nname;
MaskPtr=newmask;
OutPtr=out;


for(count=0;count<strlen(newmask);count++) {
	if((char) *MaskPtr == '?') {					/* match single character */
		*OutPtr++=*NewMaskPtr++;
	}
	else
	{
		*OutPtr++=*MaskPtr;
 	}

	MaskPtr++;
}

return;
}			

void ToHex(uint32_t hex,char *buf) {
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

return;
}
int TokenizeLine(char *linebuf,char *tokens[][BUF_SIZE],char *split) {
char *TokenPtr;
int TokenCount=0;
char *DestPtr;
char *splitptr;

memset(tokens,0,BUF_SIZE);

TokenPtr=linebuf;

DestPtr=tokens[0];

while(*TokenPtr != 0) {
	splitptr=split;			/* point to split characters */

	while(*splitptr != 0) {

		if(*TokenPtr == *splitptr++) {	/* end of token */
			TokenPtr++;
			TokenCount++;

			*DestPtr=0;		/* put null at end of token */

			memset(tokens[TokenCount],0,BUF_SIZE);

			DestPtr=tokens[TokenCount];
			break;
   		}
 	 }	

	 *DestPtr++=*TokenPtr++;
 }

TokenCount++;
*tokens[TokenCount]=0;

return(TokenCount);
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

int TCPReadLine(int socket,char *buf) {
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

int GetValueFromTimeString(char *str) {
char *TimeStringPtr;
char *CopyPtr;
char *TemporaryBuffer[BUF_SIZE];
int total;

TimeStringPtr=str;
CopyPtr=TemporaryBuffer;

while(*TimeStringPtr++ != 0) {		/* until end */

	*CopyPtr=*TimeStringPtr++;
	
	if((*CopyPtr == 'y') || (*CopyPtr == 'MaskPtr') || (*CopyPtr == 'd') || (*CopyPtr == 'h') || (*CopyPtr == 'MaskPtr') || (*CopyPtr == 's')) {
		*CopyPtr=0;		/* overwrite time specifier */

		if(*TimeStringPtr == 'y') total += (atoi(TemporaryBuffer) * SECONDS_IN_YEAR);
		if(*TimeStringPtr == 'MaskPtr') total += (atoi(TemporaryBuffer) * SECONDS_IN_MONTH);
		if(*TimeStringPtr == 'd') total += (atoi(TemporaryBuffer) * SECONDS_IN_DAY);
		if(*TimeStringPtr == 'h') total += (atoi(TemporaryBuffer) * SECONDS_IN_HOUR);
		if(*TimeStringPtr == 'MaskPtr') total += (atoi(TemporaryBuffer) * SECONDS_IN_MINUTE);
		if(*TimeStringPtr == 's') total += atoi(TemporaryBuffer);

		CopyPtr=TemporaryBuffer;
	}
	else
	{
		CopyPtr++;
	}
}

return(total);
}

void CreateTimeString(int time,char *TimeStringBuffer) {
char *TemporaryBuffer[BUF_SIZE];

if(time >= SECONDS_IN_YEAR) {
	sprintf(TemporaryBuffer,"%dy ",time / SECONDS_IN_YEAR);
	strcat(TimeStringBuffer,TemporaryBuffer);
	time -= SECONDS_IN_YEAR/(time / SECONDS_IN_YEAR);
}

if(time >= SECONDS_IN_MONTH) {
	sprintf(TemporaryBuffer,"%dm ",time / SECONDS_IN_MONTH);
	strcat(TimeStringBuffer,TemporaryBuffer);

	time -= SECONDS_IN_MONTH/(time / SECONDS_IN_MONTH);
}

if(time > SECONDS_IN_DAY) {
	sprintf(TemporaryBuffer,"%dd ",time / SECONDS_IN_DAY);
	strcat(TimeStringBuffer,TemporaryBuffer);

	time -= SECONDS_IN_DAY/(time / SECONDS_IN_DAY);
}

if(time >= SECONDS_IN_HOUR) {
	sprintf(TemporaryBuffer,"%dh ",time / SECONDS_IN_HOUR);
	strcat(TimeStringBuffer,TemporaryBuffer);

	time -= SECONDS_IN_HOUR/(time / SECONDS_IN_HOUR);
}

if(time >= SECONDS_IN_MINUTE) {
	sprintf(TemporaryBuffer,"%dm ",time / SECONDS_IN_MINUTE);
	strcat(TimeStringBuffer,TemporaryBuffer);

	time -= SECONDS_IN_MINUTE/(time / SECONDS_IN_MINUTE);
}

if(time > SECONDS_IN_DAY) {
	sprintf(TemporaryBuffer,"%ds ",time);
	strcat(TimeStringBuffer,TemporaryBuffer);
}

return;
}

void RemoveNewLine(char *line) {
char *lineptr=line+(strlen(line)-1);

if(*lineptr == '\n') *lineptr=0;
lineptr--;
if(*lineptr == '\r') *lineptr=0;
}


