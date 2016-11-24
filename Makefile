CC=cc
CFLAGS=-I.

parsing:
	$(CC) -std=c99 -Wall src/parsing.c src/mpc.c src/util.c -ledit -lm -o parsing

clean:
	rm -f parsing
