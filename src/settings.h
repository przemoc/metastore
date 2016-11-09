/*
 * Copyright (C) 2013 Przemyslaw Pawelczyk <przemoc@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; only version 2 of the License is applicable.
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
	char *metafile;          /**< path to the file containing the metadata */
	bool do_mtime;           /**< should mtimes be corrected? */
	bool do_emptydirs;       /**< should empty dirs be recreated? */
	bool do_removeemptydirs; /**< should new empty dirs be removed? */
	bool do_git;             /**< should .git dirs be processed? */
	int  restrict_to_fs;     /**< should processing only be done within this FS? */
	unsigned long long int device_id;      /**< device id storage if do_one_fs */
};

/* Convenient typedef for immutable settings */
typedef const struct metasettings msettings;


/**
 * enum for identifying additional command line arguments without single character [-x] option
 * respective flags (e.g. settings.restrict_to_fs) will be set to the
 * corresponding value
 */
enum _CommandLineOption {
  CommandLineOption_None = 0,		/**< option not set */
  CommandLineOption_RestrictFS,		/**< value for --one-file-system*/
};

#endif /* SETTINGS_H */
