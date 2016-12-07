all:
	gcc -g -Wall -I./include -c src/ticket.c -lpthread 
	gcc -g -I./include -c src/list.c -o list.o -Wall
	gcc -g -I./include -c schedule.c -o schedule.o -Wall -lrt -lnuma -lpthread -std=gnu99
	gcc list.o schedule.o ticket.o -L/home/kantonia/scheduler/ -g -o schedule -lmctop -lpthread -lnuma -lrt

clean:
	rm -f schedule
