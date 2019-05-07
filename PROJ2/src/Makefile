CC=gcc
CFLAGS=-I. -D_REENTRANT -lpthread
DEPS = constants.h sope.h types.h argProcessing.h fifoMaker.h accounts.h bankoffice.h logfileopeners.h
OBJ_SERV = log.c server.c fifoMaker.c argProcessing.c accounts.c bankoffice.c logfileopeners.c
OBJ_USER = log.c user.c fifoMaker.c argProcessing.c logfileopeners.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

user: $(OBJ_USER)
	$(CC) -o $@ $^ $(CFLAGS)

server: $(OBJ_SERV)
	$(CC) -o $@ $^ $(CFLAGS)

clean: rm -f user server
