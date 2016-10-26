all:
	gcc -g schedule.c -o schedule -Wall -lmctop -lnuma -lpthread -std=c99

clean:
	rm -f schedule
