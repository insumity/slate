all:
	gcc -g -Wall -I./include -c src/ticket.c -lpthread 
	gcc -g -I./include -c src/list.c -o list.o -Wall
	gcc -g -I./include -c src/slate_utils.c -o slate_utils.o -Wall
	gcc -g -I./include -c src/schedule.c -o schedule.o -Wall -std=gnu99

	gcc -g -I./include  -c src/use_linux_scheduler.c -o use_linux_scheduler.o 
	gcc slate_utils.o use_linux_scheduler.o -o use_linux_scheduler -lpthread

	gcc list.o slate_utils.o schedule.o ticket.o -L/home/kantonia/scheduler/ -g -o schedule -lmctop -lpthread -lnuma -lrt

	rm -f *.o

clean:
	rm -f schedule use_linux_scheduler *.o
