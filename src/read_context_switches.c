#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "read_context_switches.h"

bool starts_with(char* whole_string, char* looking_for)
{
  while (*looking_for != '\0') {
    if (*whole_string == '\0' || *whole_string != *looking_for) {
      return false;
    }
    whole_string++;
    looking_for++;
  }

  return true;
}

int get_line_number_of_context_switches(bool isVoluntary, FILE* fp) {
  rewind(fp);

  int lines = 1;
  char* line = malloc(1000);
  while (fgets(line, 1000, fp) != NULL) {
    if (isVoluntary && starts_with(line, "voluntary")) {
      return lines;
    }
    else if (!isVoluntary && starts_with(line, "nonvoluntary")) {
      return lines;
    }
    lines++;
  }

  return -1;
}

char* get_nth_line(int n, FILE* fp) {
  rewind(fp);

  int lines = 1;

  char* line = malloc(1000);
  while (fgets(line, 1000, fp) != NULL) {
    if (lines == n) {
      return line;
    }
    lines++;
  }

  return NULL;
}

long long extract_number(char* line) {

  long long res = 0;

  while (*line != '\0') {
    if (*line >= '0' && *line <= '9') {
      res = res * 10 + (*line - '0');
    }
    line++;
  }

  return res;
}

long long read_context_switches(bool isVoluntary, pid_t pid) {
  char filename[1000];
  sprintf(filename, "/proc/%ld/status", (long int) pid);

  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("file was closed");
    perror("What happened");
    return -1;
  }
  
  char* line;

  int voluntary_line = get_line_number_of_context_switches(true, fp);
  int unvoluntary_line = get_line_number_of_context_switches(false, fp);

  if (voluntary_line == -1 || unvoluntary_line == -1) {
    return -1;
  }

  if (isVoluntary) {
    line = get_nth_line(voluntary_line, fp);
  }
  else {
    line = get_nth_line(unvoluntary_line, fp);
  }

  if (line == NULL) {
    return -1;
  }

  long long result = extract_number(line);
  free(line);
  fclose(fp);
  return result;
}

/* int main(int argc, char* argv[]) */
/* { */

/*   read_context_switches(true, atoi(argv[1])); */

/*   return 0; */
/* } */

