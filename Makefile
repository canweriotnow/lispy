CC=cc
CFLAGS=-I.

parsing:
	$(CC) -std=c99 -Wall parsing.c mpc.c -ledit -lm -o parsing
