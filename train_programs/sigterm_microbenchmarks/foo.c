#include <stdio.h>

int main()
{

  char* line = "1, 0, 3184025, 844, 145, 122, 82576014, 3173849175, 26235004621, 30, 862, 0, 52.45, 38.125, 45.79, 47.495, 11, 0";
    

  long long new_values[11];
  double socket_values[4];
  int final_result, loc, rr;
  sscanf(line, "%d, %d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lf, %lf, %lf, %lf, %lld, %d", &loc, &rr,
        &new_values[0], &new_values[1], &new_values[2], &new_values[3], &new_values[4],
	&new_values[5], &new_values[6], &new_values[7], &new_values[8], &new_values[9],
	           &socket_values[0], &socket_values[1], &socket_values[2], &socket_values[3], &new_values[10],
	          &final_result);
  
  return 0;
}

