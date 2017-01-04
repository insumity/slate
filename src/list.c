#include <stdlib.h>
#include <stdio.h>
#include "list.h"

list* create_list() {
  list* l = malloc(sizeof(list));
  l->head = NULL;
  int res = sem_init(&(l->lock), 0, 1);
  if (res == -1) {
    perror("couldn't create list");
  }
  return l;
}

void list_add(list* l, void* key, void* data) {
  struct node *head = l->head;
  
  struct node *tmp = malloc(sizeof(struct node));
  sem_wait(&(l->lock));
  tmp->key = key;
  tmp->data = data;
  tmp->next = head;

  l->head = tmp;
  sem_post(&(l->lock));
}

void list_traverse(list* l, void (*print_value)(void*, void *)) {
  sem_wait(&(l->lock));

  if (l->head == NULL) {
    sem_post(&(l->lock));
    return;
  }

  struct node *tmp = l->head;
  while (tmp != NULL) {
    print_value(tmp->key, tmp->data);
    tmp = tmp->next;
  }
  sem_post(&(l->lock));
}

int list_remove(list* l, void* key, int (*compare)(void*, void*)) {
  sem_wait(&(l->lock));
  if (l->head == NULL) {
    sem_post(&(l->lock));
    return 0;
  }

  struct node *tmp = l->head;
  if (compare(tmp->key, key)) {
    l->head = l->head->next;
    free(tmp);
    sem_post(&(l->lock));
    return 1;
  }

  while (tmp->next != NULL) {
    if (compare(tmp->next->key, key)) {
      struct node *t = tmp->next;
      tmp->next = tmp->next->next;
      free(t);
      sem_post(&(l->lock));
      return 1;
    }
    tmp = tmp->next;
  }

  sem_post(&(l->lock));
  return 0;
}

void* list_get_value(list* l, void* key, int (*compare)(void*, void*)) {
  sem_wait(&(l->lock));
  if (l->head == NULL) {
    sem_post(&(l->lock));
    return NULL;
  }

  struct node *tmp = l->head;
  while (tmp != NULL) {
    if (compare(tmp->key, key)) {
      sem_post(&(l->lock));
      return tmp->data;
    }
    tmp = tmp->next;
  }

  sem_post(&(l->lock));
  return NULL;
}

void** list_get_all_values(list* l, void* key, int (*compare)(void*, void*), int* number_of_elements) {
  sem_wait(&(l->lock));
  if (l->head == NULL) {
    *number_of_elements = 0;
    sem_post(&(l->lock));
    return NULL;
  }

  int cnt = 0;
  struct node *tmp = l->head;
  while (tmp != NULL) {
    if (compare(tmp->key, key)) {
      cnt++;
    }
    tmp = tmp->next;
  }
  *number_of_elements = cnt;

  if (cnt == 0) {
    sem_post(&(l->lock));
    return NULL;
  }

  void** elements = malloc(sizeof(void *) * cnt);
  cnt = 0;
  tmp = l->head;
  while (tmp != NULL) {
    if (compare(tmp->key, key)) {
      elements[cnt] = tmp->data;
      cnt++;
    }
    tmp = tmp->next;
  }


  sem_post(&(l->lock));
  return elements;
}

int list_elements(list* l) {
  sem_wait(&(l->lock));
  int cnt = 0;
  struct node *tmp = l->head;
  while (tmp != NULL) {
    cnt++;
    tmp = tmp->next;
  }
  sem_post(&(l->lock));
  return cnt;
}

