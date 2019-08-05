
CC = gcc

all:
	$(CC) -w -omud attack.c command.c  database.c getconfig.c glob.c help.c lookup.c monster.c mud.c password.c shutdown.c spell.c string.c  user.c -lcrypt
	$(CC) -c genpass.c -o genpass.o -lcrypt
	$(CC) -o genpass genpass.o -lcrypt

win32:
	$(CC) -c -w attack.c command.c  database.c getconfig.c glob.c help.c lookup.c monster.c mud.c password.c shutdown.c spell.c string.c  user.c
	$(CC) -o mud.exe attack.o ban.o chmod.o chown.o command.o copy.o copyfile.o createobject.o createroom.o createuser.o deleteobject.o description.o drop.o force.o getconfig.o getpass.o give.o glob.o go.o help.o inventory.o kill.o login.o look.o lookup.o monster.o move.o mud.o password.o pickup.o pose.o quit.o rename.o reset.o score.o sendmsg.o shutdown.o spell.o strtrunc.o take.o token.o updatedatabase.o updateuser.o  wall.o who.o  -lws2_32 -lcrypt -Wl,-Map -Wl,mud.map
	$(CC) -c genpass.c -o genpass.o
	$(CC) -o genpass.exe genpass.o -lcrypt

clean:
	rm mud genpass attack.o ban.o chmod.o chown.o command.o copy.o copyfile.o createobject.o createroom.o createuser.o deleteobject.o description.o drop.o force.o getconfig.o getpass.o give.o glob.o go.o help.o inventory.o itoa.o kill.o login.o look.o lookup.o monster.o move.o mud.o password.o pickup.o pose.o quit.o rename.o reset.o score.o sendmsg.o shutdown.o spell.o strtrunc.o take.o token.o updatedatabase.o updateuser.o  wall.o who.o


