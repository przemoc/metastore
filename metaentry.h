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

#include <stdbool.h>

/* Data structure to hold all metadata for a file/dir */
struct metaentry {
	struct metaentry *next; /* For the metahash chains */
	struct metaentry *list; /* For creating additional lists of entries */

	char *path;
	unsigned int pathlen;

	char *owner;
	char *group;
        mode_t mode;
	time_t mtime;
        long mtimensec;

	unsigned int xattrs;
	char **xattr_names;
	ssize_t *xattr_lvalues;
	char **xattr_values;
};

#define HASH_INDEXES 1024

/* Data structure to hold a number of metadata entries */
struct metahash {
        struct metaentry *bucket[HASH_INDEXES];
        unsigned int count;
};

/* Create a metaentry for the file/dir/etc at path */
struct metaentry *mentry_create(const char *path);

/* Recurses opath and adds metadata entries to the metaentry list */
void mentries_recurse_path(const char *opath, struct metahash **mhash);

/* Stores a metaentry list to a file */
void mentries_tofile(const struct metahash *mhash, const char *path);

/* Creates a metaentry list from a file */
void mentries_fromfile(struct metahash **mhash, const char *path);

/* Searches haystack for an xattr matching xattr number n in needle */
int mentry_find_xattr(struct metaentry *haystack,
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

/* Compares two metaentries and returns an int with a bitmask of differences */
int mentry_compare(struct metaentry *left,
		   struct metaentry *right,
		   bool do_mtime);

/* Compares lists of real and stored metadata and calls pfunc for each */
void mentries_compare(struct metahash *mhashreal,
                      struct metahash *mhashstored,
                      void (*pfunc)(struct metaentry *real,
                                    struct metaentry *stored,
                                    int cmp),
                      bool do_mtime);

