all:
	gcc -g -Wall -I./include -c src/ticket.c -lpthread 
	gcc -g -I./include -c list.c -o list.o -Wall
	gcc -g -I./include -c schedule.c -o schedule.o -Wall -lrt -lnuma -lpthread -std=gnu99
	gcc list.o schedule.o ticket.o -L/home/kantonia/scheduler/ -lmctop -g -o schedule -lpthread -lnuma -lrt

clean:
	rm -f schedule
