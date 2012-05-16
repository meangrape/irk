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

#ifndef __SECURITY_SECURITY_H
#define __SECURITY_SECURITY_H

#include <common/strhash.h>

/**
  This module contains all the wrapper functions for security calls.

  This is intended to make auditing the security specific components of this
  software really easy. All major security operations should be defined
  within this module to vastly reduce the audit footprint.
 */

enum security_error_codes {
  S_OK = 0x0,
  S_ERROR = 0x01,
  S_OWNER_NOT_ROOT = 0x02,
  S_GROUP_NOT_ROOT = 0x03,
  S_WORLD_WRITABLE = 0x04,
  S_GROUP_WRITABLE = 0x05,
};


/**
  Verify the security of every directory from / to 'dirname'

  This will walk upwards from the given file, verifying that every path in
  its ancestry is secure and valid. The idea is to ensure that no path
  can be altered by a non root user.

  An optional cache can also be provided in order to speed up processing
  and prevent unnecessary checks.

  Note:
    that calling this function will change the current working dir in its
    process of checking directories.

  Arguments:
    dirname: The path to check.
    cache:
      The strhash object that can be used to save resources and prevent
      extra checking. If this is null then no extra checking will be done.

  Returns:
    S_OK if all directories are okay or a value from security_error_codes if
    a parent is not secure. There is no way to recover the name of the
    directory that failed the check.
  */
int
security_check_path(
    const char *dirname,
    strhash *cache);


/**
  Checks the basic properties of the stat structure.

  This will check the properties of the stat structure in order to ensure
  that all the settings are nice and secure. This will check that the owner
  is root, the group is root, and that the object is not world writable.

  Arguments:
    filename:
      The filename of the object being worked on. This is only used for error
      messages and is not used directly in any other way.
    s: The stat structure holding the returned data.

  Returns:
    An ORed list from security_error_codes, where S_OK is secure, and all
    other values are some form of error or insecurity.
 */
int
security_review_stat(
    const char *filename,
    const struct stat *s);



#endif
