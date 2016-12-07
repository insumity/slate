#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

char** read_line(char* line, char* policy, int* num_id)
{
    int program_cnt = 0;
    char tmp_line[300];

    if (line[strlen(line) - 1] == '\n') {
      line[strlen(line) - 1 ] = '\0'; // remove new line character
    }
    strcpy(tmp_line, line);
    int first_time = 1;
    char* word = strtok(line, " ");
    *num_id = atoi(word);
    printf("The num id is: %d\n", *num_id);
    word = strtok(NULL, " ");
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
    strtok(NULL, " "); // skip the number
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

    return program;
}


typedef struct {
  char **program;
  char* id;
  FILE* fp;
} thread_args;

void* execute_process(void* arg)
{
  printf("I'm running by : %ld\n", syscall(SYS_gettid));
  thread_args* a = (thread_args *) arg;
  char **program = a->program;
  char* id = a->id;
  FILE* fp = a->fp;
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  pid_t pid = fork();
  if (pid == 0) {
    execv(program[0], program); // SPPEND TWO HOURS ON this, if you use execve instead with envp = {NULL} it doesn't work
    perror("Couldn't run execv\n");
  }

  int status;
  waitpid(pid, &status, 0);
  clock_gettime(CLOCK_MONOTONIC, &end);
  
  double *elapsed = malloc(sizeof(double));
  *elapsed = (end.tv_sec - start.tv_sec);
  *elapsed += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
  fprintf(fp, "%s\t%ld\t%lf\tprogram\n", id, (long) syscall(SYS_gettid), *elapsed);
  return elapsed;
}

int main(int argc, char* argv[])
{
  FILE* fp = fopen(argv[1], "r");
  FILE* outfp = fopen(argv[2], "w");
  char line[300];
  pthread_t threads[100]; // up to 100 applications
  int programs = 0;
  
  while (fgets(line, 300, fp)) {
    if (line[0] == '#') {
      continue; // the line is commented
    }

    char policy[100];
    int num_id;
    char** program = read_line(line, policy, &num_id);
    line[0] = '\0';

    thread_args * args = malloc(sizeof(thread_args));
    args->program= program;
    args->id = malloc(sizeof(char) * 100);
    args->fp = outfp;
    sprintf(args->id, "%d", num_id);
    int res = pthread_create(&threads[programs], NULL, execute_process, args);
    programs++;
  }

  int i;
  for (i = 0; i < programs; ++i) {
    void* result;
    pthread_join(threads[i], &result);
  }

  fclose(fp);
  fclose(outfp);

  return 0;
}

