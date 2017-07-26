#include <stdlib.h>
#include <stdio.h>


int main()
{
  int INIT_SIZE = 1024 * 1024 * 256;

  long total_sum = 0;
  int times = 256;
  for (int k = 0; k < 1000; ++k) {
    int SIZE = INIT_SIZE / times;
    int* foo =  malloc(sizeof(int) * SIZE);

    for (int i = 0; i < SIZE; ++i) {
      foo[i] = 0;
    }

    long sum = 0;
    for (int i = 0; i < SIZE; ++i) {
      sum += (i * 343 + 19) % 234910;
    }
    total_sum += sum;
  }

  return 0;
}

