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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <common/logging.h>
#include <common/strhash.h>
#include <security/security.h>

static int
security_check_path_stat(
  const char *filename,
  strhash *cache);

static int
security_check_path_inner(
    char *filename,
    strhash *cache);

int security_is_okay_sigil = 0;

static int
security_check_path_stat(
  const char *filename,
  strhash *cache)
{
  struct stat s;

  if (cache != NULL && strhash_haskey(cache, filename)) {
    log_debug("Found successful cache for %s, skipping checks", filename);
    return S_OK;
  }

  if (lstat(filename, &s) != 0) {
    log_debug("stat(%s) error: %s", filename, strerror(errno));
    log_security(
        "Unable to stat directory (%s): %s",
        filename,
        strerror(errno));
    return S_ERROR;
  }

  // Perform our normal security checks. This will ensure the directory
  // is owned by root:root, and that it is not world writable, and if this
  // returns anything other than OK then return that value.
  int return_value = security_review_stat(filename, &s);
  if (return_value != S_OK) {
    return return_value;
  }

  // If this directory is not a link then we can safely exit now as well.
  if ( (S_ISLNK(s.st_mode)) == 0) {
    log_debug("Caching successful results for %s", filename);
    if (cache != NULL &&
        strhash_add(cache, filename, &security_is_okay_sigil) == NULL) {
      // We take no other action here since this only impacts caching and
      // not actual functionality.
      log_debug("strhash_add() error: %s", filename);
    }
    return return_value;
  }

  // We need to make sure we keep a copy of the current working directory
  // so that we can return to it if needed (like during recursion.)
  int current_dir_fd = open(".", O_RDONLY);
  if (current_dir_fd < 0) {
    log_debug("open(\".\") error: %s", strerror(errno));
    return S_ERROR;
  }

  // Change into the linked path.
  log_debug("Changing directory to %s", filename);
  if (chdir(filename) != 0) {
    log_debug("chdir(%s) error: %s", filename, strerror(errno));
    if (close(current_dir_fd)) {
      log_debug("close(%d) error: %d", current_dir_fd, strerror(errno));
    }
    return S_ERROR;
  }

  // Get its current working directory.
  char *new_wd = getwd(NULL);
  if (new_wd == NULL) {
    log_debug("getwd(NULL) error: %s", strerror(errno));
    if (close(current_dir_fd)) {
      log_debug("close(%d) error: %d", current_dir_fd, strerror(errno));
    }
    return S_ERROR;
  }

  return_value = security_check_path_inner(new_wd, cache);

  // Change back to the directory we were in before starting this faf.
  if (fchdir(current_dir_fd)) {
    log_debug("fchdir(%s) failed: %s", current_dir_fd, strerror(errno));
    free(new_wd);
    if (close(current_dir_fd)) {
      log_debug("close(%d) error: %d", current_dir_fd, strerror(errno));
    }
    return S_ERROR;
  }

  // Cache the results if everything is okay.
  if (return_value == S_OK && cache != NULL) {
    log_debug("Caching successful results for %s", filename);
    if (strhash_add(cache, filename, &security_is_okay_sigil) == NULL) {
      // We take no other action here since this only impacts caching and
      // not actual functionality.
      log_debug("strhash_add() error: %s", filename);
    }
  }

  return return_value;
}


/*
  This wraps the security_check_path function to limit strdups.

  This gets called when the string is already a mutable memory location
  that can safely be destroyed. The outer function wraps this call in a
  strdup to make the interface cleaner.
 */
static int
security_check_path_inner(
    char *filename,
    strhash *cache)
{
  int return_value = S_OK;
  // Now walk each and every component attempting to stat and then chdir
  // into it.
  for (char *current = filename; /* No check */ ; current++) {
    if (*current == '\0') {
      return_value = security_check_path_stat(filename, cache);
      break;
    } else if (*current == '/') {
      if (current == filename) {
        return_value = security_check_path_stat("/", cache);
      } else {
        *current = '\0';
        return_value = security_check_path_stat(filename, cache);
        *current = '/';
      }
      if (return_value != S_OK)
        break;
    }
  }

  return return_value;
}


int
security_check_path(
    const char *filename,
    strhash *cache)
{
  // Make a copy of the filename structure so that we can modify it while
  // splitting out the components.
  char *buffer = strdup(filename);
  if (buffer == NULL) {
    log_debug("strdup() error: %s", strerror(errno));
    return S_ERROR;
  }

  int return_value = security_check_path_inner(buffer, cache);
  free(buffer);
  return return_value;
}


int
security_review_stat(
    const char *filename,
    const struct stat *s)
{
  // Error codes start with S_OK (0) and get bits added to them for various
  // security conditions.
  int return_code = S_OK;

  if (s->st_uid != 0) {
    log_security("security_warning: %s is not owned by user root.", filename);
    return_code |= S_OWNER_NOT_ROOT;
  }

  if (s->st_gid != 0) {
    log_security("security_warning: %s is not owned by group root.", filename);
    return_code |= S_GROUP_NOT_ROOT;
  }

  if ( (s->st_mode & S_IWOTH) != 0) {
    log_security("security_warning: %s is world writable.", filename);
    return_code |= S_WORLD_WRITABLE;
  }

  if ( (s->st_mode & S_IWGRP) != 0) {
    log_security("security_warning: %s is group writable.", filename);
    return_code |= S_GROUP_WRITABLE;
  }

  // FIXME(brady): Add support for POSIX ACLs.

  return return_code;
}
