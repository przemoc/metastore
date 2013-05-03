/*
 * Copyright (C) 2013 Przemyslaw Pawelczyk <przemoc@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

/* Data structure to hold metastore settings */
struct metasettings {
	char *metafile;    /* path to the file containing the metadata */
	bool do_mtime;     /* should mtimes be corrected? */
	bool do_emptydirs; /* should empty dirs be recreated? */
	bool do_git;       /* should .git dirs be processed? */
};

/* Convenient typedef for immutable settings */
typedef const struct metasettings msettings;

#endif /* SETTINGS_H */
