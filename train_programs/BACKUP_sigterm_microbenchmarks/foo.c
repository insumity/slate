#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
  srand(38);

  long sum = 0;
  for (int i = 0; i < 10; ++i) {
    int k = rand();
    printf("%d\n", k);
  }



  return 0;
}

