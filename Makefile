CC = gcc
OBJFILES=attack.o command.o  database.o error.o getconfig.o help.o lookup.o monster.o mud.o password.o shutdown.o spell.o string.o  user.o
OUTFILE=mud
ifeq ($(OS),Windows_NT)
	FLAGS = -llibcrypt
	OUTFILE += ".exe"
else
	FLAGS= -lcrypt
endif

mud: $(OBJFILES)
	$(CC) -w $(OBJFILES) -o $(OUTFILE) $(FLAGS)

$(OBJFILES): %.o: %.c
	$(CC) -c -w -Iheaders $< -o $@

clean:
	rm $(OUTFILE) *.o

