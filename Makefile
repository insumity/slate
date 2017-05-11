cc = gcc -g -Wall -pedantic -I ./include -I ./include/classifier -I ./train_data
cpp = g++ -g -Wall -pedantic -I ./include -I ./include/classifier -I ./train_data -fpermissive -std=c++11
objects = python_classify.o read_context_switches.o moving_average.o read_counters.o heuristic.o heuristicX.o heuristic_MCTOP.o heuristic_split.o heuristic_greedy.o heuristic_rr_lat.o ticket.o list.o slate_utils.o schedule.o use_linux_scheduler.o read_memory_bandwidth.o get_tids.o


#classifier_objects = maxent.o lbfgs.o owlqn.o sgd.o

all: schedule use_linux_scheduler

schedule: $(objects) $(classifier_objects)
	$(cpp) python_classify.o read_context_switches.o read_memory_bandwidth.o get_tids.o moving_average.o list.o read_counters.o heuristic.o heuristicX.o heuristic_MCTOP.o heuristic_split.o heuristic_greedy.o heuristic_rr_lat.o slate_utils.o schedule.o ticket.o $(classifier_objects) -L/home/kantonia/scheduler/ -o schedule -lboost_serialization -lmlpack -larmadillo -lmctop -lpthread -lnuma -lrt -lpapi

use_linux_scheduler: use_linux_scheduler.o slate_utils.o
	$(cc) slate_utils.o use_linux_scheduler.o -o use_linux_scheduler -lmctop -lpthread -lnuma -L/localhome/kantonia/slate/papi/src/ -lpapi

python_classify.o: src/python_classify.c include/python_classify.h
	$(cc) -c src/python_classify.c


moving_average.o: src/moving_average.cpp include/moving_average.h
	$(cpp) -c src/moving_average.cpp

read_counters.o: src/read_counters.c include/read_counters.h
	$(cc) -c src/read_counters.c -lpthread

read_context_switches.o: src/read_context_switches.c include/read_context_switches.h
	$(cc) -c src/read_context_switches.c

read_memory_bandwidth.o: src/read_memory_bandwidth.c include/read_memory_bandwidth.h
	$(cc) -c src/read_memory_bandwidth.c

get_tids.o: src/get_tids.c include/get_tids.h
	$(cc) -c src/get_tids.c

heuristic.o: src/heuristic.cpp include/heuristic.h
	$(cpp) -c src/heuristic.cpp 

heuristicX.o: src/heuristicX.cpp include/heuristicX.h
	$(cpp) -c src/heuristicX.cpp 

heuristic_MCTOP.o: src/heuristic_MCTOP.cpp include/heuristic_MCTOP.h
	$(cpp) -c src/heuristic_MCTOP.cpp

heuristic_greedy.o: src/heuristic_greedy.cpp include/heuristic_greedy.h
	$(cpp) -c src/heuristic_greedy.cpp

heuristic_split.o: src/heuristic_split.cpp include/heuristic_split.h
	$(cpp) -c src/heuristic_split.cpp

heuristic_rr_lat.o: src/heuristic_rr_lat.cpp include/heuristic_rr_lat.h
	$(cpp) -c src/heuristic_rr_lat.cpp

ticket.o: src/ticket.c include/ticket.h
	$(cc) -c src/ticket.c -lpthread

list.o: src/list.cpp include/list.h
	$(cpp) -c src/list.cpp -o list.o

slate_utils.o: src/slate_utils.c include/slate_utils.h
	$(cc) -c src/slate_utils.c -o slate_utils.o

schedule.o: src/schedule.cpp
	$(cpp) -c src/schedule.cpp -o schedule.o

use_linux_scheduler.o: src/use_linux_scheduler.c slate_utils.o
	$(cc) -c src/use_linux_scheduler.c -o use_linux_scheduler.o 

.PHONY: clean
clean:
	rm -f schedule use_linux_scheduler $(objects) 
