#define _GNU_SOURCE
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <mctop.h>
#include <mctop_alloc.h>

int* get_thread_ids(pid_t pid, int *number_of_threads) {

    char str_pid[100];
    str_pid[0] = '\0';
    sprintf(str_pid, "%d", pid);
    
    char directory[200];
    directory[0] = '\0';
    strcat(directory, "/proc/");
    strcat(directory, str_pid);
    strcat(directory, "/task/");

    
    DIR *d = opendir(directory);
    struct dirent *dir;

    int num_threads = 0;

    if (d)  {
        while ((dir = readdir(d)) != NULL)  {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue;
            }
            num_threads++;
        }

        int* result = malloc(sizeof(int) * num_threads);
        int thread_cnt = 0;

        d = opendir(directory);
        if (d == NULL) {
            perror("couldn't open the directory!");
            return NULL;
        }

        while ((dir = readdir(d)) != NULL)  {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue;
            }

            result[thread_cnt] = atoi(dir->d_name);
            thread_cnt++;
        }
        *number_of_threads = thread_cnt;
        
        closedir(d);

        return result;
    }
    else {
        perror("couldn't open the directory!");
        return NULL;
    }

    assert(0);
}

int main(int argc, char* argv[]) {

    printf("Provide it with the following input: \"POLICY FULL_PATH_PROGRAM\"\n");

    mctop_t* topo = mctop_load(NULL);
    if (topo)   {
        mctop_print(topo);
    }

    size_t num_nodes = mctop_get_num_nodes(topo);
    size_t num_cores = mctop_get_num_cores(topo);
    size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
    size_t num_hwc_per_socket = mctop_get_num_hwc_per_socket(topo);
    size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);
               
    printf("%zu %zu %zu %zu %zu\n", num_nodes, num_cores, num_cores_per_socket, num_hwc_per_socket, num_hwc_per_core);


    mctop_alloc_policy x;

    while (1) {
        char policy[100];
        char program[200];
        scanf("%s %s", policy, program);
        printf("--[%s] .. [%s]\n", policy, program);

        if (strcmp(policy, "MCTOP_ALLOC_NONE") == 0) {
            ;
        }
        else if (strcmp(policy, "MCTOP_ALLOC_SEQUENTIAL") == 0) {

        }
        else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_HWCS") == 0) {
            ;
        }


        pid_t pid = fork();
        if (pid == 0) {
            execv(program, (char *[]) {&program[0], NULL});
            perror("execv didn't work!\n"); 
        }

        usleep(1000);

        int number_of_threads = 0;
        int* threads = get_thread_ids(pid, &number_of_threads);

        int i;
        for (i = 0; i < number_of_threads; ++i) {
            printf("i>>> %d\n", threads[i]);
            /*cpu_set_t set;

            CPU_ZERO(&set);
            CPU_SET(atoi(argv[2]), &set );
            if (sched_setaffinity(threads[i], sizeof( cpu_set_t ), &set ))   {
                perror( "sched_setaffinity" );
                return NULL;
            }*/
        }
    }
                  
    
    mctop_free(topo);

    return 0;
}

