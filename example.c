#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

void print_thread_id(pthread_t id)
{
        size_t i;
            for (i = sizeof(i); i; --i)
                 printf("%02x", *(((unsigned char*) &id) + i - 1));
}

void *print_hello(void *threadid)
{
    long tid;
    tid = (long) threadid;
    printf("Hello world! It's me, thread #%ld! with tid: ", tid); 
    print_thread_id(pthread_self());
    printf("\n");

    int i = 0;
    while (1) {
        i++;
    }
    int sum = i;

    pthread_exit(NULL);
}

int main()
{
    int NUM_OF_THREADS = 0;
    pthread_t threads[NUM_OF_THREADS];

    printf("PROCCESS: %d\n", getpid());

    int i;
    for (i = 0; i < NUM_OF_THREADS; ++i) {
        long t = i;
        int rc = pthread_create(&threads[i], NULL, print_hello, (void *) t);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    int j = 0;
    while (1) {
        j++;
    }
    int sum = j - 10;

    pthread_exit(NULL);

    return 0;
}

