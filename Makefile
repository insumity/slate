all:
	gcc -g -Wall -I./include -c src/ticket.c -lpthread 
	gcc -g -I./include -c src/list.c -o list.o -Wall
	gcc -g -I./include -c src/slate_utils.c -o slate_utils.o -Wall
	gcc -g -I./include -c src/schedule.c -o schedule.o -Wall -lrt -lnuma -lpthread -std=gnu99
	gcc list.o slate_utils.o schedule.o ticket.o -L/home/kantonia/scheduler/ -g -o schedule -lmctop -lpthread -lnuma -lrt
	rm -f *.o

clean:
	rm -f schedule *.o
