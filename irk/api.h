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

#ifndef __IRK_API_H
#define __IRK_API_H

#ifndef IRK_MODULE_DATA_DEFINED
#define IRK_MODULE_DATA_DEFINED
// TODO(brady): Document me!
typedef void module;
// TODO(brady): Document me!
typedef void module_data;
#endif

// TODO(brady): Document me!
enum irk_value_type {
  IRK_STRING,
  IRK_INT,
  IRK_DOUBLE
};


/**
  Creates a new module object.

  This will take the base settings from the 'mod' module object and copy them
  into a new module object, returning the results. This is used when a single
  library actually exports more than one module. This is often the case when
  a single module might export data in lots of different time cycles.

  Note:
    This does NOT copy modifications to the module object, it simply sets up
    a new object exactly like it had been initialized and passed into
    module_init().

  Arguments:
    mod: The initialized module object to copy from.

  Returns:
    A initialized module object on success, or NULL on failure.
 */
module *
new_module_object(
    module *mod);

/**
  Sets the root path that this module will export data in.

  This will register the given path as the root for all variables exported by
  this module. The value passed in here will be copied as it may not be safe
  to rely on the pointer being passed in.

  This will also clean up some common mistakes with the argument. If the path
  passed in does not start with a '/' one will be added, and if it ends with a
  '/' it will be removed. However, some values will be rejected as they
  are not valid paths.

  Note:
    This path will be prepended to all variables that this module exports.

  Arguments:
    mod:
      The module reference that this is associated with. This is passed in
      to the irk_module_init function that is called to setup the module.
    path:
      The path that will be prepended to all values.

  Returns:
    0 on success.
    EINVAL if mod is NULL.
 */
int
set_root_path(
    module *mod,
    char *path);


/**
  Sets the function that will be used for initial data collection.

  The callback defined by this function will only ever be called once. All
  data that it collects will remain active until either the program finishes,
  or a timer_callback happens.

  Functionally this can be used to collect information that will never update,
  such as the boot time of the machine, firmware of hardware, etc.

  It is also possible to use this call in order to setup data for later calls,
  such as opening a file or socket, etc. This is not recommended as it may
  complicate the irk runtime, or even introduce instability. If not carefully
  considered it may also introduce non-consistent monitoring output.

  Arguments:
    mod:
      The module reference that this is associated with. This is passed in
      to the irk_module_init function that is called to setup the module.
    initial:
      The callback function that will be called after the module is
      initialized and before any timer_callback's (if configured) are called.
    initial_data:
      An optional value that will be preserved and passed into
      the function specified by 'initial' as 'user_data'. This data will
      not be used in any other way by irk.

  Returns:
    0 on success.
    EINVAL if mod is NULL.
 */
int
register_initial_callback(
    module *mod,
    module_data *(*initial)(void *user_data),
    void *initial_data);


/**
  Sets the function that will be called on timer updates for this module.

  This will ensure that this module will be updated roughly on a cycle defined
  by the variable 'cycle_time'. There is no assurances that the time between
  runs will match cycle_time, as various things can delay execution.

  Arguments:
    mod:
      The module reference that this is associated with. This is passed in
      to the irk_module_init function that is called to setup the module.
    timer: The callback function that will be called when the timer triggers.
    timer_data:
      An optional value that will be preserved and passed into
      the function specified by 'timer' as 'user_data'. This data will
      not be used in any other way by irk.
    cycle_time: The time that should pass between calls.

  Returns:
    0 on success,
    EINVAL if cycle_time is not valid or mod is NULL.
 */
int
register_timer_callback(
    module *mod,
    module_data *(*timer)(void *user_data),
    void *timer_data,
    struct timeval *cycle_time);


/**
  Sets the function to call when the client requests refreshed data.

  The client can request data that is more up to date than what is cached in
  memory. This is not common but may be supported if people want to be able to
  monitor at super high resolution for a short period.

  As an added protection a module can set a minimum refresh time via this call
  as well. That will ensure that at a specific amount of time has elapsed
  before a new refresh is allowed. Typically this is not necessary, however if
  a module is expensive to the machine, or can possibly take a long time to
  refresh then it is nice to be able to disable this feature.

  Note:
    You can completely disable refreshing for this module by specifically
    setting this function to NULL.

  Arguments:
    mod:
      The module reference that this is associated with. This is passed in
      to the irk_module_init function that is called to setup the module.
    refresh:
      The function that will be called when a client specifically requests
      refreshed data for this module.
    refresh_data:
      An optional value that will be preserved and passed into
      the function specified by 'refresh' as 'user_data'. This data will
      not be used in any other way by irk.
    minimum_time:
      The minimum time that must pass between calls to the refresh function.

  Returns:
    0 on success.
    EINVAL if minimum_time is not valid or mod is NULL.
 */
int
register_refresh_callback(
    module *mod,
    module_data *(*refresh)(void *user_data),
    void *refresh_data,
    struct timeval *minimum_time);


/**
  Sets this module up to not appear in the default view.

  By default all modules, and therefor all data gathered by modules will be
  exported to any user requesting '/'. Falling this function on your module
  will ensure that this module's data is only ever exposed if the user
  specifically requests it.

  Arguments:
    mod:
      The module reference that this is associated with. This is passed in
      to the irk_module_init function that is called to setup the module.

  Returns:
    0 on success,
    EINVAL if mod is NULL.
 */
int
remove_from_default_view(
    module *mod);



#endif
