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

#include <common/config.h>
#include <common/logging.h>
#include <common/strhash.h>
#include <master/module.h>
#include <security/security.h>



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
  // Initialize a cache to store cached security information in.
  strhash *cache = strhash_init(511);
  if (cache == NULL) {
    log_debug("strhash_init(511) error: %s", strerror(errno));
    return errno;
  }

  // Walk the results.
  while (true) {
    errno = 0;
    FTSENT *f = fts_read(d);
    if (errno != 0) {
      log_error(
          "Error reading modules directory (%s): %s",
          path,
          strerror(errno));
    }

    if (f == NULL) {
      // End of the file stream with no errors. Clear up the cache and
      // return success.
      strhash_destroy(cache, NULL);
      return 0;
    }

    // On recursion fts returns FTS_DC when it returns to a directory. We
    // simply ignore this type.
    if (f->fts_info == FTS_DP) {
      continue;
    }

    // Don't bother loading anything that isn't even a file or directory.
    if ( (f->fts_info & (FTS_F | FTS_D)) == 0) {
      log_debug("Not considering module in non-file: %s\n", f->fts_path);
      continue;
    }

    // We only load files with the ".irkmod" extension. This ensures that we
    // do not accidentally load some unrelated library with a horrible
    // _init() function.
    if ((f->fts_info & FTS_F) != 0 &&
        (f->fts_namelen < 7 ||
         strncmp(f->fts_name + f->fts_namelen - 7, ".irkmod", 7))) {
      log_info("Not loading file (%s): bad extension.", f->fts_path);
      continue;
    }

    // SECURITY CHECKS
    if (security_check_path(f->fts_path, cache) != S_OK) {
      log_debug("security_check_path(%s) failed.", f->fts_path);
      log_error("Can not load module file %s, it is not secure.", f->fts_path);
      break;
    }

    // Beyond checking permissions we do nothing with directories.
    if ( (f->fts_info & FTS_D) != 0) {
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
  strhash_destroy(cache, NULL);
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
