all:
	g++ -c -g gather_data.cpp -std=c++11
	gcc -c -g read_counters.c
	gcc -c -g read_context_switches.c
	gcc -c -g get_tids.c 
	g++ -g read_counters.o get_tids.o gather_data.o read_context_switches.o -o gather_data
