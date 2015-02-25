/*
 * Platform-specific interface abstraction layer
 *
 * Copyright (C) 2007 David Härdeman <david@hardeman.nu>
 *
 *      This program is free software; you can redistribute it and/or modify it
 *      under the terms of the GNU General Public License as published by the
 *      Free Software Foundation version 2 of the License.
 *
 *      This program is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

//----------------------------------------------------------------------

#ifdef __linux__

#include <sys/xattr.h>

char *
canonical_path_get (const char *path)
{
    return canonicalize_file_name(path);
}

ssize_t
xattr_get (const char* path, const char* name, void * value, size_t size)
{
    return getxattr(path, name, value, size);
}

ssize_t
xattr_list (const char* path, char * list, size_t size)
{
    return listxattr(path, list, size);
}

int
xattr_remove_link (const char * path, const char * name)
{
    return lremovexattr(path, name);
}

int
xattr_set_link (const char * path, const char * name, const void * value,
                size_t size)
{
    return lsetxattr(path, name, value, size, XATTR_CREATE);
}
#endif

//----------------------------------------------------------------------

#ifdef __FreeBSD__

#include <sys/extattr.h>

char *
canonical_path_get (const char *path)
{
    return realpath(path, NULL);
}

ssize_t
xattr_get (const char* path, const char* name, void * value, size_t size)
{
    return extattr_get_file(path, EXTATTR_NAMESPACE_USER, name, value, size);
}

ssize_t
xattr_list (const char* path, char * list, size_t size)
{
    return extattr_list_file(path, EXTATTR_NAMESPACE_USER, list, size);
}

int
xattr_remove_link (const char * path, const char * name)
{
    return extattr_delete_link(path, EXTATTR_NAMESPACE_USER, name);
}

int
xattr_set_link (const char * path, const char * name, const void * value,
                size_t size)
{
    int res = 0;

    ssize_t length = extattr_set_link(path, EXTATTR_NAMESPACE_USER, name,
                                      value, size);
    if (length < 0)
    {
        res = -1;
    }

    return res;
}

#endif

//----------------------------------------------------------------------
