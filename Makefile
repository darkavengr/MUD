CC = gcc
CCFLAGS=-w
OBJFILES = attack.o command.o  database.o error.o getconfig.o help.o lookup.o monster.o mud.o password.o shutdown.o spell.o string.o  user.o
OUTFILE=mud

ifeq ($(OS),Windows_NT)
	FLAGS = -llibcrypt
	OUTFILE += ".exe"
else
	FLAGS= -lcrypt
endif

mud: $(OBJFILES)
	$(CC) $(CCFLAGS) $(OBJFILES) -o $(OUTFILE) $(FLAGS)

attack.o:
	$(CC) $(CCFLAGS) -c attack.c

command.o:
	$(CC) $(CCFLAGS) -c command.c

database.o:
	$(CC) $(CCFLAGS) -c database.c

error.o:
	$(CC) $(CCFLAGS) -c error.c

getconfig.o:
	$(CC) $(CCFLAGS) -c getconfig.c

help.o:
	$(CC) $(CCFLAGS) -c help.c

lookup.o:
	$(CC) $(CCFLAGS) -c lookup.c

monster.o:
	$(CC) $(CCFLAGS) -c monster.c

mud.o:
	$(CC) $(CCFLAGS) -c mud.c

password.o:
	$(CC) $(CCFLAGS) -c password.c

shutdown.o:
	$(CC) $(CCFLAGS) -c shutdown.c

spell.o:
	$(CC) $(CCFLAGS) -c spell.c

string.o:
	$(CC) $(CCFLAGS) -c string.c

user.c:
	$(CC) $(CCFLAGS) -c user.c

clean:
	rm $(OUTFILE) *.o





