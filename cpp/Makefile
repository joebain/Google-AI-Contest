CC=g++

all: MyBot tcp

clean:
	rm -rf *.o MyBot

MyBot: MyBot.o PlanetWars.o

tcp: tcp.c
	gcc tcp.c -o tcp
