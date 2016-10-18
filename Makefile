all:
	gcc -g schedule.c -o schedule -Wall -lmctop -lnuma

clean:
	rm -f schedule
