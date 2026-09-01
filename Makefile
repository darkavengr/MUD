CC = gcc
OBJFILES=user.o attack.o command.o world.o error.o getconfig.o help.o lookup.o monster.o mud.o password.o shutdown.o spell.o string.o
OUTFILE=mud

GENOUTFILE=genpass
GENOBJFILES= hashpassword.o genpass.o

ifeq ($(OS),Windows_NT)
	FLAGS = -llibcrypt
	OUTFILE += ".exe"
	GENOUTFILE += ".exe"
else
	FLAGS= -lcrypt -lsqlite3
endif

all: mud genpass

mud: $(OBJFILES) hashpassword.o
	$(CC) -w $(OBJFILES)  hashpassword.o -o $(OUTFILE) $(FLAGS)

genpass: $(GENOBJFILES)
	$(CC) -w genpass.o hashpassword.o -o $(GENOUTFILE) $(FLAGS)

$(OBJFILES) $(GENOBJFILES): %.o: %.c
	$(CC) -c -w -Iheaders $< -o $@

clean:
	rm $(GENOUTFILE) $(OUTFILE) *.o

