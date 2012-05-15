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
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>

#include <master/module.h>


module *
new_module_object(
    module *mod)
{
  if (mod == NULL) {
    syslog(
        LOG_WARNING,
        "Unknown module: Call to remove_from_default_view where mod == NULL");
    errno = EINVAL;
    return NULL;
  }

  // FIXME(brady)
  return NULL;
}

int
set_root_path(
    module *mod,
    char *path)
{
  if (mod == NULL) {
    syslog(
        LOG_WARNING,
        "Unknown module: Call to remove_from_default_view where mod == NULL");
    errno = EINVAL;
    return EINVAL;
  }

  if (path == NULL) {
    syslog(
        LOG_WARNING,
        "%s(%p): Call to remove_from_default_view where path == NULL",
        mod->module_file->filename,
        mod);
    errno = EINVAL;
    return EINVAL;
  }

  // FIXME(brady)
  return -1;
}


int
register_initial_callback(
    module *mod,
    module_data *(*initial)(void *user_data),
    void *initial_data)
{
  if (mod == NULL) {
    syslog(
        LOG_WARNING,
        "Unknown module: Call to remove_from_default_view where mod == NULL");
    errno = EINVAL;
    return EINVAL;
  }

  if (mod->register_initial_callback_called) {
    syslog(
        LOG_WARNING,
        "Module %s(%p): Duplicate call to register_initial_callback()",
        mod->module_file->filename,
        mod);
  }

  mod->register_initial_callback_called = true;
  mod->initial = initial;
  mod->initial_data = initial_data;
  return 0;
}


int
register_timer_callback(
    module *mod,
    module_data *(*timer)(void *user_data),
    void *timer_data,
    struct timeval *cycle_time)
{
  if (mod == NULL) {
    syslog(
        LOG_WARNING,
        "Unknown module: Call to remove_from_default_view where mod == NULL");
    errno = EINVAL;
    return EINVAL;
  }

  if (mod->register_timer_callback_called) {
    syslog(
        LOG_WARNING,
        "Module %s(%p): Duplicate call to register_timer_callback_called()",
        mod->module_file->filename,
        mod);
  }

  mod->register_timer_callback_called = true;
  mod->timer = timer;
  mod->timer_data = timer_data;
  return 0;
}


int
register_refresh_callback(
    module *mod,
    module_data *(*refresh)(void *user_data),
    void *refresh_data,
    struct timeval *minimum_time)
{
  if (mod == NULL) {
    syslog(
        LOG_WARNING,
        "Unknown module: Call to remove_from_default_view where mod == NULL");
    errno = EINVAL;
    return EINVAL;
  }

  if (mod->register_refresh_callback_called) {
    syslog(
        LOG_WARNING,
        "Module %s(%p): Duplicate call to register_refresh_callback()",
        mod->module_file->filename,
        mod);
  }

  mod->register_refresh_callback_called = true;
  mod->refresh = refresh;
  mod->refresh_data = refresh_data;
  return 0;
}


int
remove_from_default_view(
    module *mod)
{
  if (mod == NULL) {
    syslog(
        LOG_WARNING,
        "Unknown module: Call to remove_from_default_view where mod == NULL");
    errno = EINVAL;
    return EINVAL;
  }

  if (mod->in_default_view == false) {
    syslog(
        LOG_WARNING,
        "Module %s(%p): Duplicate call to remove_from_default_viewi()",
        mod->module_file->filename,
        mod);
  }

  mod->in_default_view = false;
  return 0;
}
