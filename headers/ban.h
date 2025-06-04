#include "size.h"

#ifndef BAN_H
#define BAN_H
typedef struct {
 char *ipaddress[BUF_SIZE];
 struct ban *next;
} ban;
#endif

