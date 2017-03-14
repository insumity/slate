all:
	g++ -c gather_data.cpp -std=c++11 -D PIN
	gcc -c read_counters.c
	gcc -c read_context_switches.c
	gcc -c get_tids.c 
	g++ read_counters.o get_tids.o gather_data.o read_context_switches.o -o gather_data
