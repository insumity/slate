all: gather_data.cpp
	gcc -g loc2.c -o loc2 -lpthread -lnuma
	gcc -g loc3.c -o loc3 -lpthread -lnuma
	gcc -g rr.c -o rr -lpthread -lnuma
	gcc -g loc.c -o loc -lpthread -lnuma
	gcc -g sort.c -o sort -lpthread -lnuma
	gcc -g empty_threads.c -o empty_threads -lpthread -lnuma

	gcc -g lock.c -o lock -lpthread 
	gcc -g rr2.c -o rr2 -lpthread -lnuma
	g++ -c -g gather_data.cpp -std=c++11
	gcc -c -g read_counters.c
	gcc -c -g read_context_switches.c
	gcc -c -g read_memory_bandwidth.c
	gcc -c -g get_tids.c 
	g++ -g read_memory_bandwidth.o read_counters.o get_tids.o gather_data.o read_context_switches.o -o gather_data

	gcc -DLOCKFREE_RES -D_GNU_SOURCE -O3 -Wall -I./clht/include clht_mb.c -o clht_mb -L./clht/external/lib -L./clht/ -lrt -lpthread -lm -lclht -lssmem -g


classifier: classifier.cpp
	g++ classifier.cpp -std=c++11 -lboost_serialization -lmlpack -larmadillo -o classifier -g

clean:
	rm -f *.o
