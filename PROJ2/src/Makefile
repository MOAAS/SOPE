CC=gcc
CFLAGS=-I.
DEPS = constants.h sope.h types.h
OBJ = log.c server.c user.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

make: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
