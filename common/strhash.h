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

#ifndef __COMMON_STRHASH_H
#define __COMMON_STRHASH_H

#include <stdbool.h>

// The strhash.c file uses an actual struct to store data, other classes
// are only ever allowed to see void in order to prevent them from messing
// with internals.
#ifndef STRHASH_IS_NOT_VOID
typedef void strhash;
#else
typedef struct strhash strhash;
#endif


/**
  An implementation of the djb2 hash function.

  This is a simple implementation of the djb2 hash function which is used
  to has input strings in order to put them in the hash table.

  Arguments:
    string: A '\0' terminated character string to use for hashing.

  Returns:
    A hash.
 */
unsigned long
strhash_djb2(
    const char *string);


/**
  Creates a strhash structure using malloc.

  This will create a strhash structure and pass it back to the caller. The
  results of this function, if not NULL, will need to be destroyed using the
  strhash_destroy call otherwise a memory leak will be created.

  Arguments:
    table_size: The initial size of the hash table. This should be roughly
                three times larger than the expected data size or there will
                be a performance penalty on all operations. Ideally this should
                also be a prime number.

  Returns:
    An initialized strhash structure.
 */
strhash *
strhash_init(
    int table_size);


/**
  Adds data to the strhash table.

  This will add value, indexed by key to the strhash table, unless a value
  already exists at 'key'. The return value of this function is a bit tricky.
  The return value will be either the value added, or the 'value' of the
  previous element with the same key.

  In order to explain this a bit here is a code snippet:
  char * key = "some key";
  void * value = <data>;
  void * added_value = strhash_add(s, key, value);
  if (value_added == NULL)
    // Error adding.
  else if (value_added != value)
    // Key already existed
  else
    // Added successfully.

  Arguments:
    s: The strhash object created using strhash_init().
    key: The '\0' terminated key to add the value at.
    value: The value to add. This is void * so it may effectively be anything.

  Returns:
    NULL on error, 'value' on success, or a previous added value if the key
    already exists in the hash table.
 */
void *
strhash_add(
    strhash *s,
    const char *key,
    void *value);


/**
  Returns true if a value is associated with the key already.

  This will search the given key to see if it is associated with a value
  in the table already. If so this will return true, of not this will return
  false.

  Arguments:
    s: The strhash object created using strhash_init().
    key: The '\0' terminated key to check for.

  Returns:
    true if the key exists, false if does not.
 */
bool
strhash_haskey(
    strhash *s,
    const char *key);


/**
  Returns the value associated with the given key.

  This will look up the given key, returning its value if its found, NULL if
  not found. Note that this means that not found, and a value of NULL become
  indistinguishable. If you wish to use this function and may have values
  such as NULL then use a sigil.

  Arguments:
    s: The strhash object created using strhash_init().
    key: The '\0' terminated key to get the value for.

  Returns:
    The value associated with key or NULL if no key by that name exists.
 */
void *
strhash_get(
    strhash *s,
    const char *key);


/**
  Frees the memory associated with the given strhash structure.

  This will walk through freeing all allocated memory associated with the
  given strhash structure. If the caller wants to ensure that the values
  get freed as well then a function can be provided to do that. If this is
  NULL then no value freeing will be performed.

  Arguments:
    s: The strhash object created using strhash_init().
    free_func: A function that will be called on each value as the table
               is being freed. If this is NULL then no values freeing will
               be untouched.
 */
void
strhash_destroy(
    strhash *s,
    void (*free_func)(void *ptr));


#endif
