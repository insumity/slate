#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{

  pid_t pid = fork();


  if (pid == 0) {
    char* program[] = {"/home/kantonia/kyotocabinet-1.2.76/bin/kccachetest_MUTEX", "wicked", "-th", "6", "1000000", NULL};
    execv(program[0], program);
  }

  int status;
  waitpid(pid, &status, 0);

  
  return 0;
}
