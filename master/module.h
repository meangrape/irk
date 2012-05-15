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

#ifndef __DAEMON_MODULE_H
#define __DAEMON_MODULE_H

#include <sys/stat.h>
#include <stdbool.h>

// Ensures that the api header does not overwrite this definition using
// void types. We do not let modules know the contents of these structures
// to keep modules from mucking with them.
#define IRK_MODULE_DATA_DEFINED
typedef struct module_data_node module_data_node;
typedef struct module_data module_data;
typedef struct module_file module_file;
typedef struct module module;

#include <irk/api.h>


struct module_data_node {
  char *key;
  enum irk_value_type type;
  union {
    char *string;
    int64_t i;
    double d;
  } value;;
  module_data_node *next;
};


struct module_data {
  module_data_node *head;
};


struct module_file {
  /** The file name that this module was loaded from. */
  char *filename;

  /**
    The inode of the file. We use this to tell if the file has been changed.
   */
  dev_t inode;

  /**
    The last modification time of this file, again used to tell if the file
    has changed since we loaded it.
   */
  time_t modified_time;
};


struct module {
  /** The module_file that this module was loaded from. */
  module_file *module_file;

  /** Registered path that this module has claimed. */
  char *registered_path;

  /**
    Function to call in order to get the initial data.

    This will be called only after the module is loaded. This can be used
    to collect information that can not change, like for example the time
    that a machine booted, the version of its firmware, etc.
   */
  module_data *(*initial)(void *user_data);

  /**
    Stored data used with 'initial'.

    This is user provided and will never be touched by anything inside of irk.
   */
  void *initial_data;

  /**
    Function to call in order to get timer data.

    This will be called at periodic intervals in order to update the cached
    data associated with this module. If this is not defined (NULL) then no
    timer updates will happen.

    The value of 'user_data' will always be 'timer_data' which is passed
    in at module initialization time.
   */
  module_data *(*timer)(void *user_data);

  /**
    Stored data used with 'timer'.

    This is user provided and will never be touched by anything inside of irk.
   */
  void *timer_data;

  /**
    Stores the interval that should be kept between timer_callback calls.

    If timer is NULL then this will be unused.
   */
  struct timeval timer_delay;

  /**
    Function to call in order to get data on refresh.

    This will be called at periodic intervals in order to update the cached
    data associated with this module. If this is not defined (NULL) then no
    timer updates will happen.

    The value of 'user_data' will always be 'timer_data' which is passed
    in at module initialization time.
   */
  module_data *(*refresh)(void *user_data);

  /**
    Stored data used with 'refresh'.

    This is user provided and will never be touched by anything inside of irk.
   */
  void *refresh_data;

  /**
    Minimum time that should pass between allowed refreshes.

    Since refresh can be triggered by a web user, and may in turn cause
    an expensive data collection cycle, irk allows a minimum time to be set
    between refreshes (globally). This can be used with any module that is
    expensive to collect, or which causes impact on the local machine.

    By default the time between refreshes is not limited.
   */
  struct timeval refresh_minimum;

  /**
    Make the output from this module visible in default scans.

    This is a boolean value (0 for false) that specifies if the data in this
    module should be displayed by a default query. This can be used to hide
    excessive, or uninteresting modules from regular display, but keep them
    accessible in the case where they are still desired.
   */
  bool in_default_view;

  /** Set if register_initial_callback was called. */
  bool register_initial_callback_called;

  /** Set if register_timer_callback was called. */
  bool register_timer_callback_called;

  /** Set if register_refresh_callback was called. */
  bool register_refresh_callback_called;

  /** Linked list used for module_file tracking. */
  module *next;
};


/**
  Loads all the library modules from the given path.

  This will load all the library module files in a given path, then schedule
  the initial data collection.

  Arguments:
    path: The path to load library module in.

  Returns:
    0 on success.
    EINVAL if path is in someway invalid.

    FIXME: This needs cleaned up
 */
int
modules_load(
    const char *path,
    const module_file **existing_modules,
    int *modules_length);

#endif
