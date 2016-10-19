all:
	gcc -g schedule.c -o schedule -Wall -lmctop -lnuma -lpthread

clean:
	rm -f schedule
