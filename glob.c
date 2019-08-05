/*
  Adventure MUD server 
 
  (c) Copyright Matthew Boote 2018, All rights reserved blah blah blah etc etc etc 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "defines.h"

extern char *noroom;
char *mudserver[BUF_SIZE];
int mudport;
int objectresettime;
int databaseresettime;
int banresettime;
int userresettime;
int configsavetime;
int databasebackup;
int allowplayerkilling;
int allownewaccounts;
int monsterresettime;
int databasememorysize;

int pointsforwarrior=400;
int pointsforhero=800;
int pointsforchampion=1600;
int pointsforsuperhero=3200;
int pointsforenchanter=6400;
int pointsforsorceror=128000;
int pointsfornecromancer=256000;
int pointsforlegend=512000;
int pointsforwizard=1024000;

char *mudnomem="mud:out of memory\n";

char *isbuf;
int issuecount;
int allownewaccounts;
int lastroom;

char *noobject="Object not found\r\n";
char *notyet="You can't do that yet\r\n";



