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

#ifndef OS_IF_H
#define OS_IF_H

extern char *
canonical_path_get (const char *path);

extern ssize_t
xattr_get (const char* path, const char* name, void * value, size_t size);

extern ssize_t
xattr_list (const char* path, char * list, size_t size);

extern int
xattr_remove_link (const char * path, const char * name);

// FIXME: return value difference

// FIXME: we removed the "int flags" parameter at the end, to which we passed
// XATTR_CREATE before

extern int
xattr_set_link (const char * path, const char * name, const void * value,
            size_t size);

#endif /* OS_IF_H */
