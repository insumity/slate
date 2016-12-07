
int* get_children_thread_ids(pid_t pid, int* number_of_threads) {
  //pstree -p 31796 | grep -P -o '\([0-9]+\)'
  FILE *fp;
  char path[1035];

  char command[500];
  command[0] = '\0';
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

// migrates all memory belonging to pid from all memory
// nodes to memory node "to"
int migrate_memory(int pid, int to) {
  long _all = (1 << 8) - 1;
  long _to = 1 << to;
  return migrate_pages(pid, 30, &_all, &_to);
}

// returns the size of a file in bytes
off_t fsize(char *file) {
  struct stat filestat;
  if (stat(file, &filestat) == 0) {
    return filestat.st_size;
  }
  return 0;
}


