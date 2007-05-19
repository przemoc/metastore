/*
 * Various functions to work with meta entries.
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

void mentries_recurse_path(const char *opath, struct metaentry **mhead);

void mentries_tofile(const struct metaentry *mhead, const char *path);

void mentries_fromfile(struct metaentry **mhead, const char *path);

int mentry_find_xattr(struct metaentry *heystack,
                      struct metaentry *needle,
                      int n);

#define DIFF_NONE  0x00
#define DIFF_OWNER 0x01
#define DIFF_GROUP 0x02
#define DIFF_MODE  0x04
#define DIFF_TYPE  0x08
#define DIFF_MTIME 0x10
#define DIFF_XATTR 0x20
#define DIFF_ADDED 0x40
#define DIFF_DELE  0x80
void mentries_compare(struct metaentry *,
                      struct metaentry *,
                      void (*printfunc)(struct metaentry *,
                                        struct metaentry *,
                                        int)
                     );

