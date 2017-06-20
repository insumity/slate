#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>
#include <signal.h>
#include <clht.h>


clht_t* global_ht;


int main()
{
  srand(time(NULL));

  global_ht = clht_create(100000);
  clht_gc_thread_init(global_ht, 1);

  clht_addr_t* key1 = malloc(sizeof(clht_addr_t));
  int* key1_int = malloc(sizeof(int));
  *key1 = (clht_addr_t) key1_int;
  *key1_int = 548;

  
  clht_val_t* v1 = malloc(sizeof(clht_val_t));
  int* v1_int = malloc(sizeof(int));
  *v1 = (clht_val_t) v1_int;
  *v1_int = 138990;

  clht_put(global_ht, *key1, *v1);


  clht_addr_t* key2 = malloc(sizeof(clht_addr_t));
  *key2 = (clht_addr_t) key1_int;

  printf("%p\n", (void *) clht_get(global_ht->ht, *key2));
  printf("%d\n", *((int *) clht_get(global_ht->ht, *key2)));

  clht_gc_destroy(global_ht);

  return 0;
}



