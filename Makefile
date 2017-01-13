
cc = gcc -g -Wall -pedantic -I./include
objects = heuristic.o heuristicX.o heuristic_MCTOP.o heuristic_split.o heuristic_greedy.o heuristic_rr_lat.o ticket.o list.o slate_utils.o schedule.o use_linux_scheduler.o

all: schedule use_linux_scheduler

schedule: $(objects)
	$(cc) heuristic.o heuristicX.o heuristic_MCTOP.o heuristic_split.o heuristic_greedy.o heuristic_rr_lat.o list.o slate_utils.o schedule.o ticket.o -L/home/kantonia/scheduler/ -o schedule -lmctop -lpthread -lnuma -lrt

use_linux_scheduler: use_linux_scheduler.o slate_utils.o
	$(cc) slate_utils.o use_linux_scheduler.o -o use_linux_scheduler -lmctop -lpthread -lnuma

heuristic.o: src/heuristic.c include/heuristic.h
	$(cc) -c src/heuristic.c 

heuristicX.o: src/heuristicX.c include/heuristicX.h
	$(cc) -c src/heuristicX.c 

heuristic_MCTOP.o: src/heuristic_MCTOP.c include/heuristic_MCTOP.h
	$(cc) -c src/heuristic_MCTOP.c 

heuristic_greedy.o: src/heuristic_greedy.c include/heuristic_greedy.h
	$(cc) -c src/heuristic_greedy.c 

heuristic_split.o: src/heuristic_split.c include/heuristic_split.h
	$(cc) -c src/heuristic_split.c 

heuristic_rr_lat.o: src/heuristic_rr_lat.c include/heuristic_rr_lat.h
	$(cc) -c src/heuristic_rr_lat.c 

ticket.o: src/ticket.c include/ticket.h
	$(cc) -c src/ticket.c -lpthread

list.o: src/list.c include/list.h
	$(cc) -c src/list.c -o list.o

slate_utils.o: src/slate_utils.c include/slate_utils.h
	$(cc) -c src/slate_utils.c -o slate_utils.o

schedule.o: src/schedule.c
	$(cc) -c src/schedule.c -o schedule.o -std=gnu99

use_linux_scheduler.o: src/use_linux_scheduler.c slate_utils.o
	$(cc) -c src/use_linux_scheduler.c -o use_linux_scheduler.o 

.PHONY: clean
clean:
	rm -f schedule use_linux_scheduler $(objects) 
