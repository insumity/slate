#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "list.h"

typedef struct {
  void *k, *v;
} pair;

pair create_pair(int key, int value)
{
  int *k = malloc(sizeof(int));
  int *v = malloc(sizeof(int));
  *k = key;
  *v = value;
  pair p;
  p.k = k;
  p.v = v;
  return p;
}

int compare(void* key1, void* key2) 
{
  return *((int *) key1) == *((int *) key2);
}

int main() {
  list* l = create_list();
  assert(l != NULL);

  pair p = create_pair(58, 73);
  void* old_pv = p.v;
  list_add(l, p.k, p.v);
  assert(list_get_value(l, p.k, compare) == p.v);
  assert(*((int *) list_get_value(l, p.k, compare)) == 73);

  p = create_pair(324, 99);
  list_add(l, p.k, p.v);
  assert(list_get_value(l, p.k, compare) == p.v);
  
  p = create_pair(58, -1);
  assert(list_get_value(l, p.k, compare) == old_pv);

  p = create_pair(38911, 9829);
  list_add(l, p.k, p.v);
  assert(list_get_value(l, p.k, compare) == p.v);

  p = create_pair(324, -1);
  assert(list_get_value(l, p.k, compare) != NULL);

  p = create_pair(325, -1);
  assert(list_get_value(l, p.k, compare) == NULL);

  p = create_pair(58, -1);
  assert(list_get_value(l, p.k, compare) != NULL);
  assert(list_remove(l, p.k, compare) == 1);
  assert(list_get_value(l, p.k, compare) == NULL);
  assert(list_remove(l, p.k, compare) == 0);

  return 0;
}

