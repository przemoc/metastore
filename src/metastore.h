/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Main functions of the program.
 *
 * Copyright (C) 2007-2008 David Härdeman <david@hardeman.nu>
 * Copyright (C) 2013-2015 Przemyslaw Pawelczyk <przemoc@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; only version 2 of the License is applicable.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#define ACTION_DUMP  0x04
#define ACTION_SAVE  0x10
#define ACTION_VER   0x08
#define ACTION_HELP  0x80

/* Action masks */
#define ACTIONS_READING 0x07
#define ACTIONS_WRITING 0x70

/* Possible build defines */
#ifndef   NO_XATTR
# define  NO_XATTR 0
#endif /* NO_XATTR */

/* Messages */
#define NO_XATTR_MSG "no XATTR support"

#endif /* METASTORE_H */
