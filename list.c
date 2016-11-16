#include <stdlib.h>
#include <stdio.h>
#include "list.h"

list* create_list() {
  list* l = malloc(sizeof(list));
  l->head = NULL;
  return l;
}

void list_add(list* l, void* key, void* data) {
  struct node *head = l->head;
  
  struct node *tmp = malloc(sizeof(struct node));
  tmp->key = key;
  tmp->data = data;
  tmp->next = head;

  l->head = tmp;
}

int list_remove(list* l, void* key, int (*compare)(void*, void*)) {
  if (l->head == NULL) {
    return 0;
  }

  struct node *tmp = l->head;
  if (compare(tmp->key, key)) {
    l->head = l->head->next;
    free(tmp);
    return 1;
  }

  while (tmp->next != NULL) {
    if (compare(tmp->next->key, key)) {
      struct node *t = tmp->next;
      tmp->next = tmp->next->next;
      free(t);
      return 1;
    }
    tmp = tmp->next;
  }

  return 0;
}

void* list_get_value(list* l, void* key, int (*compare)(void*, void*)) {
  if (l->head == NULL) {
    return NULL;
  }

  struct node *tmp = l->head;
  while (tmp != NULL) {
    if (compare(tmp->key, key)) {
      return tmp->data;
    }
    tmp = tmp->next;
  }

  return NULL;
}

