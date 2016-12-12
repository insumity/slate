#include <stdio.h>
#include <pthread.h>

void* foo2(void *fd)
{
  int foo[2] = {1, 53};
  long sum = 0;
  for (int j = 0; j < 2000000000; ++j) {
    foo[j % 2]++;
    sum++;
  }

    for (int j = 0; j < 2000000000; ++j) {
    foo[j % 2]++;
    sum++;
  }

      for (int j = 0; j < 2000000000; ++j) {
    foo[j % 2]++;
    sum++;
  }

        for (int j = 0; j < 2000000000; ++j) {
    foo[j % 2]++;
    sum++;
  }

    for (int j = 0; j < 2000000000; ++j) {
    foo[j % 2]++;
    sum++;
  }

      for (int j = 0; j < 2000000000; ++j) {
    foo[j % 2]++;
    sum++;
  }

      printf("%d foo[0]: %d\n", sum, foo[0]);
}

int main()
{
  pthread_t foo;
  pthread_create(&foo, NULL, foo2, NULL);

  pthread_join(foo, NULL);
  return 0;
}
