#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

int* get_thread_ids(pid_t pid, int *number_of_threads) {

    char str_pid[100];
    str_pid[0] = '\0';
    sprintf(str_pid, "%d", pid);
    
    char directory[200];
    directory[0] = '\0';
    strcat(directory, "/proc/");
    strcat(directory, str_pid);
    strcat(directory, "/task/");

    printf("FINAL PATH: %s\n", directory); 

    
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

    int number_of_threads = 0;
    int* threads = get_thread_ids(atoi(argv[1]), &number_of_threads);

    int i;
    for (i = 0; i < number_of_threads; ++i) {
        printf("i>>> %d\n", threads[i]);
    }

    char* read_policy = argv[1];
    char* read_program = argv[2];

    printf("%s %s\n", read_policy, read_program);

    return 0;
}

