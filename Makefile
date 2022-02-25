CC = gcc
OBJFILES = attack.o command.o  database.o error.o getconfig.o help.o lookup.o monster.o mud.o password.o shutdown.o spell.o string.o  user.o
OUTFILE=mud

ifeq ($(OS),Windows_NT)
	FLAGS = -llibcrypt
	OUTFILE += ".exe"
else
	FLAGS= -lcrypt
endif

mud: $(OBJFILES)
	$(CC) $(OBJFILES) -o $(OUTFILE) $(FLAGS)

attack.o:
	$(CC) -c attack.c

command.o:
	$(CC) -c command.c

database.o:
	$(CC) -c database.c

error.o:
	$(CC) -c error.c

getconfig.o:
	$(CC) -c getconfig.c

help.o:
	$(CC) -c help.c

lookup.o:
	$(CC) -c lookup.c

monster.o:
	$(CC) -c monster.c

mud.o:
	$(CC) -c mud.c

password.o:
	$(CC) -c password.c

shutdown.o:
	$(CC) -c shutdown.c

spell.o:
	$(CC) -c spell.c

string.o:
	$(CC) -c string.c

user.c:
	$(CC) -c user.c

clean:
	rm $(OUTFILE) *.o





