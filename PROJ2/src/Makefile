CC=gcc
CFLAGS=-I.
DEPS = constants.h sope.h types.h
OBJ_SERV = log.c server.c
OBJ_USER = log.c user.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

user: $(OBJ_USER)
	$(CC) -o $@ $^ $(CFLAGS)

server: $(OBJ_SERV)
	$(CC) -o $@ $^ $(CFLAGS)

clean: rm -f user server
