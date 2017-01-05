
cc = gcc -g -Wall -I./include
objects = heuristic.o ticket.o list.o slate_utils.o schedule.o


all: schedule use_linux_scheduler

schedule: $(objects)
	$(cc) heuristic.o list.o slate_utils.o schedule.o ticket.o -L/home/kantonia/scheduler/ -o schedule -lmctop -lpthread -lnuma -lrt

use_linux_scheduler: use_linux_scheduler.o slate_utils.o
	$(cc) slate_utils.o use_linux_scheduler.o -o use_linux_scheduler -lmctop -lpthread -lnuma

heuristic.o: src/heuristic.c
	$(cc) -c src/heuristic.c 

ticket.o: src/ticket.c include/ticket.h
	$(cc) -c src/ticket.c -lpthread

list.o: src/list.c include/list.h
	$(cc) -c src/list.c -o list.o

slate_utils.o: src/slate_utils.c include/slate_utils.h
	$(cc) -c src/slate_utils.c -o slate_utils.o

schedule.o: src/schedule.c
	$(cc) -c src/schedule.c -o schedule.o -std=gnu99

use_linux_scheduler.o: src/use_linux_scheduler.c
	$(cc)  -c src/use_linux_scheduler.c -o use_linux_scheduler.o 

.PHONY: clean
clean:
	rm -f schedule use_linux_scheduler $(objects)
