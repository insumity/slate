#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>


int main(int argc, char* argv[])
{
    cpu_set_t set;

    CPU_ZERO( &set );
    CPU_SET(atoi(argv[2]), &set );
                           if (sched_setaffinity( atoi(argv[1]), sizeof( cpu_set_t ), &set ))
                                   {
                                               perror( "sched_setaffinity" );
                                                       return NULL;
                                                           }

    sleep(100);
    return 0;
}

