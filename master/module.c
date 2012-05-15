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
#include <fts.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <common/logging.h>
#include <master/module.h>



struct module_file_list {
  // The actual module_file structure we are storing.
  module_file data;
  struct module_file_list *next;
};


/**
  Walks through a module_file_list linked list and frees the elements.

  This will free all memory used in a module_file_list linked list, including
  the file name allocations and such.

  Arguments:
    p: The module_file_list linked list to free.
 */
static
void
free_module_file_list(
    struct module_file_list *p)
{
  while (p != NULL) {
    struct module_file_list *p_next = p->next;
    if (p->data.filename != NULL) {
      free(p->data.filename);
    }
    free(p);
    p = p_next;
  }
}


/**
  Called to add a given file to the list.

  Arguments:
    f: The read object from fts_read().
    head: A pointer to the address storing the pointer to the list.
    length: The length of the list in head.

  Returns:
    modifies head, return 0 on success, non zero on failure.

  Errors:
    ENOMEM: If malloc() returns a NULL for any reason.
 */
int
modules_load_add(
    FTSENT *f,
    struct module_file_list **head,
    int *length)
{
  struct module_file_list *m =
      (struct module_file_list *) malloc(sizeof(struct module_file_list));
  if (m == NULL) {
    errno = ENOMEM;
    return ENOMEM;
  }

  // Filename.
  m->data.filename = (char *) malloc(f->fts_pathlen + 1);
  if (m->data.filename == NULL) {
    free(m);
    errno = ENOMEM;
    return ENOMEM;
  }
  memcpy(m->data.filename, f->fts_path, f->fts_pathlen);
  m->data.filename[f->fts_pathlen] = '\0';

  // Basic file stats.
  m->data.inode = f->fts_statp->st_dev;
  m->data.modified_time = f->fts_statp->st_mtime;

  m->next = *head;
  *head = m;
  (*length)++;
  return 0;
}


/**
  Called once the directory is opened to read each file item one by one.

  Note: This function should only ever be called from modules_load_getlist.

  Arguments:
    path: The path being processed, used for error messages.
    d: The FTS structure opened in modules_load_getlist.
    list: A pointer to the pointer containing the head of the list.
    length: A pointer to an integer containing the length of the list.

  Returns:
    0 on success, or anything else on failure.
 */
static
int
modules_load_walkfiles(
    const char *path,
    FTS *d,
    struct module_file_list **list,
    int *length)
{
  // Walk the results.
  while (true) {
    errno = 0;
    FTSENT *f = fts_read(d);
    if (f == NULL) {
      if (errno == 0) {
        // End of the file stream with no errors.
        return 0;
      } else {
        log_error(
            "Error reading modules directory (%s): %s",
            path,
            strerror(errno));
        break;
      }
    }

    // If we encounter a directory in our walk then we 
    if (f->fts_info == FTS_D) {
    }

    // Don't bother loading anything that isn't even a file.
    if (f->fts_info != FTS_F) {
      log_debug("Not loading module in non-file: %s\n", f->fts_path);
      continue;
    }

    // Attempt to add this file to the list of modules that we will load.
    if (modules_load_add(f, list, length) != 0) {
      log_error(
          "Error reading fts_read() output (%s): %s",
          path,
          strerror(errno));
      break;
    }
  }

  // If we have gotten here then there has been an error somewhere.
  // Make sure we free the list we allocated, then return NULL.
  free_module_file_list(*list);
  return -1;
}


/**
  Opens the directory and starts walking through files with in it.

  Note: This function should only ever be called form modules_load.

  Arguments:
    path: The path to open and read modules from.
    list: A pointer to the pointer containing the head of the list.
    length: A pointer to an integer containing the length of the list.

  Returns:
    0 on success, anything else on failure.
 */
static
int
modules_load_getlist(
    const char *path,
    struct module_file_list **head,
    int *length)
{
  // Walk through the path structure, returning each file, one by one as
  // somewhat complicated data structures. We use this in order to simplify
  // path expansion, link following, and to get the stat of the file so we
  // can find out if it has been modified and therefor needs updated.
  // FTS_LOGICAL = Follow symlinks, return the stat on the destination.
  // FTS_NOCHDIR = Do not use chdir during this process.
  errno = 0;
  // This is a super funky definition that is necessary given how fts_open
  // accepts arguments.
  char *path_argv[2] = {(char *) path, NULL};
  FTS *d = fts_open(path_argv, (int) (FTS_LOGICAL | FTS_NOCHDIR), NULL);
  if (d == NULL) {
    log_error(
        "Unable to traverse modules directory (%s): %s",
        path,
        strerror(errno));
    return -1;
  }

  if (modules_load_walkfiles(path, d, head, length) != 0) {
    log_error(
        "Unknown error from modules_load_walkfiles() is preventing loading.");
    return -1;
  }

  if (fts_close(d)) {
    // This shouldn't ever happen since we are not using chdir.. but still its
    // good to check and log.
    log_error(
        "Error closing the modules directory (%s): %s",
        path,
        strerror(errno));
  }

  return 0;
}


int
modules_load(
    const char *path,
    const module_file **existing_modules,
    int *modules_length)
{
  if (path == NULL) {
    errno = EINVAL;
    return EINVAL;
  }

  struct module_file_list *new_list = NULL;
  int new_length = 0;
  if (modules_load_getlist(path, &new_list, &new_length) != 0) {
    // This module actually logs its own errors so we need not attempt to
    // log the error here. Instead we just return failure.
    return -1;
  }

  // FIXME!
  struct module_file_list *p = new_list;
  while (p != NULL) {
    fprintf(stderr, "loaded: %s\n", p->data.filename);
    p = p->next;
  }

  // TODO
  return 0;
}
