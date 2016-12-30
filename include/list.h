#ifndef __LIST_H
#define __LIST_H

#include <semaphore.h>

struct node {
  struct node *next;
  void *key, *data;
};

typedef struct {
  struct node* head;
  sem_t lock;
} list;

list* create_list();

void list_add(list* l, void* key, void* data);

// returns 1 when it deletes an element, 0 otherwise
int list_remove(list* l, void* key, int (*compare)(void*, void*));

void* list_get_value(list*l, void* key, int (*compare)(void*, void*));

void list_traverse(list* l, void (*print_value)(void*, void*));

#endif
