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
#include <string.h>

int* get_children_thread_ids(pid_t pid, int* number_of_threads) {
  //pstree -p 31796 | grep -P -o '\([0-9]+\)'
  FILE *fp;
  char path[1035];

  char command[500];
  strcat(command, "pstree -p ");

  char str_pid[10];
  sprintf(str_pid, "%ld", (long) pid);
  strcat(command, str_pid);
  strcat(command, " | grep -P -o '\\([0-9]+\\)'");

  fp = popen(command, "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }
  
  int cnt = 0;
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
        printf("[[[%s\n)))", path);
	cnt++;
  }
  *number_of_threads = cnt;
  cnt = 0;
  int* thread_ids = malloc(sizeof(int) * *number_of_threads);

  fp = popen(command, "r");
  while (fgets(path, sizeof(path) -1, fp) != NULL) {
    char id[10];
    strncpy(id, path + 1, strlen(path) - 3);
    id[strlen(path) - 3] = '\0';
    //printf("The id is: [[%s]]\\n", id);
    thread_ids[cnt++] = atol(id);
  }

  *number_of_threads = cnt;
  
  pclose(fp);

  return thread_ids;
}

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

mctop_alloc_policy get_policy(char* policy) {
    mctop_alloc_policy pol;
    if (strcmp(policy, "MCTOP_ALLOC_NONE") == 0) {
        pol = MCTOP_ALLOC_NONE;
    }
    else if (strcmp(policy, "MCTOP_ALLOC_SEQUENTIAL") == 0) {
        pol = MCTOP_ALLOC_SEQUENTIAL;
    }
    else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_HWCS") == 0) {
        pol = MCTOP_ALLOC_MIN_LAT_HWCS;
    }
    else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES_HWCS") == 0) {
        pol = MCTOP_ALLOC_MIN_LAT_CORES_HWCS;
    }
    else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES") == 0) {
        pol = MCTOP_ALLOC_MIN_LAT_CORES;
    }
    else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_HWCS_BALANCE") == 0) {
        pol = MCTOP_ALLOC_MIN_LAT_HWCS_BALANCE;
    }
    else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES_HWCS_BALANCE") == 0) {
        pol = MCTOP_ALLOC_MIN_LAT_CORES_HWCS_BALANCE;
    }
    else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES_BALANCE") == 0) {
        pol = MCTOP_ALLOC_MIN_LAT_CORES_BALANCE;
    }
    else if (strcmp(policy, "MCTOP_ALLOC_BW_ROUND_ROBIN_HWCS") == 0) {
        pol = MCTOP_ALLOC_BW_ROUND_ROBIN_HWCS;
    }
    else if (strcmp(policy, "MCTOP_ALLOC_BW_ROUND_ROBIN_CORES") == 0) {
        pol = MCTOP_ALLOC_BW_ROUND_ROBIN_CORES;
    }
    else if (strcmp(policy, "MCTOP_ALLOC_BW_BOUND") == 0) {
        pol = MCTOP_ALLOC_BW_BOUND;
    }
    else {
        perror("Not recognized policy\n");
        return -1;
    }

    return pol;
}

int main(int argc, char* argv[]) {

    printf("Provide it with the following input: \"POLICY FULL_PATH_PROGRAM\"\n");

    mctop_t* topo = mctop_load("lpd48core.mct");
    if (topo)   {
        mctop_print(topo);
    }

    size_t num_nodes = mctop_get_num_nodes(topo);
    size_t num_cores = mctop_get_num_cores(topo);
    size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
    size_t num_hwc_per_socket = mctop_get_num_hwc_per_socket(topo);
    size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);

    size_t num_hwc_per_processor = num_nodes * num_cores_per_socket * num_hwc_per_core;
    printf("TOTAL number of hardware contexts: %zu\n", num_hwc_per_processor);

    printf("%zu %zu %zu %zu %zu\n", num_nodes, num_cores, num_cores_per_socket, num_hwc_per_socket, num_hwc_per_core);
    const uint8_t NUMBER_OF_POLICIES = 11;
    //list threads[NUMBER_OF_POLICIES];

    int j;
    for (j = 0; j < NUMBER_OF_POLICIES; ++j) {
       // threads[j] = create_list(); 
    }

    while (1) {
        char policy[100];
	int program_cnt = 0;
	char line[300], tmp_line[300];

	fgets(line, 300, stdin);
	line[strlen(line) - 1 ] = '\0'; // remove new line character
	printf("line to be written: [[%s]]\n", line);
	strcpy(tmp_line, line);
	int first_time = 1;
	char* word = strtok(line, " ");
	while (word != NULL)  {
	  if (first_time) {
	    first_time = 0;
	    strcpy(policy, word);
	  }
	  else {
	    program_cnt++;
	  }
	  word = strtok(NULL, " ");
	}

	char** program = malloc(sizeof(char *) * (program_cnt + 1));
	first_time = 1;
	program_cnt = 0;
	word = strtok(tmp_line, " ");
	while (word != NULL)  {

	  if (first_time) {
	    first_time = 0;
     	  }
	  else {
	    program[program_cnt] = malloc(sizeof(char) * 200);
	    strcpy(program[program_cnt], word);
	    program_cnt++;
	  }
	  word = strtok(NULL, " ");
	}

	program[program_cnt] = NULL;

	printf("verify the forking with Y: \n");
	char c = getchar();
	getchar(); // read new line
	pid_t pid;
	if (c == 'Y') {
	  pid = fork();
	}
        if (pid == 0) {
            execv(program[0], program);
            perror("execv didn't work!\n"); 
        }

	printf("PID IS: %d\n", pid);
        sleep(1);

        int number_of_threads = 0;
        //get_thread_ids(pid, &number_of_threads);
	get_children_thread_ids(pid, &number_of_threads);

	printf("Number of threads: %d\n", number_of_threads);

	
        mctop_alloc_policy pol = get_policy(policy);
        /* TODO not sure about the third parameter */
        mctop_alloc_t* alloc = mctop_alloc_create(topo, number_of_threads, num_hwc_per_socket, pol);


	cpu_set_t set;
	CPU_ZERO(&set);
        for (uint hwc_i = 0; hwc_i < alloc->n_hwcs; hwc_i++)
        {
	  CPU_SET(alloc->hwcs[hwc_i], &set);
	  printf("hwc : %d\n", alloc->hwcs[hwc_i]);
	}
        printf("Where those threads should be pinned!: %d and %d\n", set, CPU_COUNT(&set));

	if (sched_setaffinity(pid, sizeof(cpu_set_t), &set)) {
	  perror("sched_setaffinity!\n");
	  return -1;
	}
    }
	
    mctop_free(topo);

    return 0;
}

