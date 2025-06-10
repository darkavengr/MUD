CC = gcc
OBJFILES=attack.o command.o  database.o error.o getconfig.o help.o lookup.o monster.o mud.o password.o shutdown.o spell.o string.o  user.o
OUTFILE=mud
GENOUTFILE=genpass

ifeq ($(OS),Windows_NT)
	FLAGS = -llibcrypt
	OUTFILE += ".exe"
	GENOUTFILE += ".exe"
else
	FLAGS= -lcrypt
endif

all: mud genpass

mud: $(OBJFILES)
	$(CC) -w $(OBJFILES) -o $(OUTFILE) $(FLAGS)

genpass:
	$(CC) -w -Iheaders genpass.c -o $(GENOUTFILE) $(FLAGS)

$(OBJFILES): %.o: %.c
	$(CC) -c -w -Iheaders $< -o $@

clean:
	rm $(GENOUTFILE) $(OUTFILE) *.o

