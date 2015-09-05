/*
 * Main functions of the program.
 *
 * Copyright (C) 2007 David Härdeman <david@hardeman.nu>
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

#ifndef METASTORE_H
#define METASTORE_H

/* Each file starts with SIGNATURE and VERSION */
#define SIGNATURE    "MeTaSt00r3"
#define SIGNATURELEN 10
#define VERSION      "\0\0\0\0\0\0\0\0"
#define VERSIONLEN   8

/* Default filename */
#define METAFILE     "./.metadata"

/* Utility defines for the action to take */
#define ACTION_APPLY 0x01
#define ACTION_DIFF  0x02
#define ACTION_SAVE  0x10
#define ACTION_HELP  0x80

/* Action masks */
#define ACTIONS_READING 0x07
#define ACTIONS_WRITING 0x70

#endif /* METASTORE_H */
