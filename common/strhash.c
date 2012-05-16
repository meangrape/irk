/*
Copyright (C) 2012 Brady Catherman

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define STRHASH_IS_NOT_VOID

#include <common/logging.h>
#include <common/strhash.h>


// TODO(brady): Document this!
struct strhash_node {
  unsigned long hash;
  struct strhash_node *next;
  void * value;
  char key[];
};


// TODO(brady): Document this!
struct strhash {
  // Stores the actual table for the strhash pointers.
  struct strhash_node ** table;

  // Size of the table.
  int table_size;

  // Keeps track of the number of items in the table.
  int items;
};


unsigned long
strhash_djb2(
    const char *string)
{
  // Daniel J. Bernstein's popular DJB2 function.
  char *string_p = (char *) string;
  unsigned long hash = 5381;
  int c;

  while ( (c = *string_p++) != 0) {
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}


strhash *
strhash_init(
    const int table_size)
{
  strhash *p = (strhash *) malloc(sizeof(strhash));
  if (p == NULL) {
    log_debug("malloc(%d) error: %s", sizeof(strhash), strerror(errno));
    return NULL;
  }

  p->table = (struct strhash_node **)
             calloc(table_size, sizeof(struct strhash_node *));
  if (p->table == NULL) {
    log_debug(
        "calloc(%d, %d) error: %s",
        table_size,
        sizeof(struct strhash_node),
        strerror(errno));
    free(p);
    return NULL;
  }

  p->table_size = table_size;
  p->items = 0;
  return p;
}


void *
strhash_add(
    strhash *s,
    const char *key,
    void *value)
{
  unsigned long hash = strhash_djb2(key);
  struct strhash_node ** p = &(s->table[hash % s->table_size]);
  while (*p != NULL) {
    if ((*p)->hash == hash && !strcmp(key, (*p)->key)) {
      // The keys are the same, we can not overwrite the value as that might
      // be creating a memory leak.
      return (*p)->value;
    }
  }

  int key_len = strlen(key);
  size_t malloc_size = sizeof(struct strhash_node) + key_len;
  struct strhash_node *n = (struct strhash_node *) malloc(malloc_size);
  if (n == NULL) {
    return NULL;
  }

  n->hash = hash;
  n->value = value;
  n->next = NULL;
  memcpy(n->key, key, key_len + 1);
  *p = n;
  s->items++;
  return value;
}


bool
strhash_haskey(
    strhash *s,
    const char *key)
{
  unsigned long hash = strhash_djb2(key);
  struct strhash_node ** p = &(s->table[hash % s->table_size]);
  while (*p != NULL) {
    if ((*p)->hash == hash && !strcmp(key, (*p)->key)) {
      // The keys are the same, we can not overwrite the value as that might
      // be creating a memory leak.
      return true;
    }
  }

  return false;
}


void *
strhash_get(
    strhash *s,
    const char *key)
{
  unsigned long hash = strhash_djb2(key);
  struct strhash_node ** p = &(s->table[hash % s->table_size]);
  while (*p != NULL) {
    if ((*p)->hash == hash && !strcmp(key, (*p)->key)) {
      // The keys are the same, we can not overwrite the value as that might
      // be creating a memory leak.
      return (*p)->value;
    }
  }

  return NULL;
}


// This function does nothing and is used in strhash_destroy.
// This function exists to make the code in strhash_destroy easier. Rather than
// checking every single loop if the 'free_func' variable is defined, it can
// just call this.
static void
do_nothing(
    void *not_used)
{
  // This space intentionally left blank.
}


void
strhash_destroy(
    strhash *s,
    void (*free_func)(void *ptr))
{
  if (free_func == NULL) {
    free_func = do_nothing;
  }

  for (int i = 0; i < s->table_size; i++) {
    struct strhash_node *p = s->table[i];
    while (p != NULL) {
      struct strhash_node *p_next = p->next;
      free_func(p->value);
      free(p);
      p = p_next;
    }
  }
  free(s);
}
