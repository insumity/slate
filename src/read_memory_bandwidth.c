#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>

#define MAX_LINE_SIZE 1000

FILE* start_reading_memory_bandwidth() {
  system("sudo pkill -9 pcm-memory.x");
  system("sudo rm -f /tmp/memory_bw.csv");
  system("sudo /localhome/kantonia/slate/pcm/pcm-memory.x 0.5 -csv=/tmp/memory_bw.csv &");

  sleep(1);
  FILE* fp = fopen("/tmp/memory_bw.csv", "r");
  return fp;
}

void close_memory_bandwidth() {
  system("sudo pkill -9 pcm-memory.x");
}

double read_memory_bandwidth(int socket, FILE* fp) {

  char last_line1[MAX_LINE_SIZE], last_line2[MAX_LINE_SIZE];
  char current_line[MAX_LINE_SIZE];

  int lines = 0;
  while (fgets(current_line, MAX_LINE_SIZE, fp) != NULL) {
    if (lines % 2 == 0) {
      strcpy(last_line1, current_line);
    }
    else {
      strcpy(last_line2, current_line);
    }
    
    lines++;
  }

  double sockets[4] = {0};
  int elements = 0;
  char *pt = strtok(last_line1, ";");
  while (pt != NULL) {
    if (elements == 13) {
      sockets[0] += atof(pt);
    }
    else if (elements == 25) {
      sockets[1] += atof(pt);
    }
    else if (elements == 37) {
      sockets[2] += atof(pt);
    }      
    else if (elements == 49) {
      sockets[3] += atof(pt);
    }      

    pt = strtok (NULL, ";");
    elements++;
  }

  elements = 0;
  pt = strtok(last_line2, ";");
  while (pt != NULL) {
    if (elements == 13) {
      sockets[0] += atof(pt);
    }
    else if (elements == 25) {
      sockets[1] += atof(pt);
    }
    else if (elements == 37) {
      sockets[2] += atof(pt);
    }      
    else if (elements == 49) {
      sockets[3] += atof(pt);
    }      

    pt = strtok (NULL, ";");
    elements++;
  }


  return sockets[socket] / 2.0;
}

/*int main(int argc, char* argv[]) {

  FILE* fp = start_reading_memory_bandwidth();
  sleep(1);

  while (1) {
    for (int i = 0; i < 4; ++i) {
      printf("Socket %d : %lf\n", i, read_memory_bandwidth(i));
      rewind(fp);
    }


    sleep(1);
  }
  fclose(fp);
  
  return 0;
  }*/

