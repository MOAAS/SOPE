CC=gcc
CFLAGS=-I.
DEPS = ../constants.h ../sope.h ../types.h
OBJ = ../log.c user.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

user: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
