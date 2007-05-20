/*
 * Main functions of the program.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>
#include <utime.h>
#include <attr/xattr.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "metastore.h"
#include "utils.h"
#include "metaentry.h"

/* Used to signal whether mtimes should be corrected */
static int do_mtime = 0;

/*
 * Prints differences between real and stored actual metadata
 * - for use in mentries_compare
 */
static void
compare_print(struct metaentry *real, struct metaentry *stored, int cmp)
{
	if (!real) {
		msg(MSG_QUIET, "%s:\tremoved\n", stored->path);
		return;
	}

	if (!stored) {
		msg(MSG_QUIET, "%s:\tadded\n", real->path);
		return;
	}

	if (cmp == DIFF_NONE) {
		msg(MSG_DEBUG, "%s:\tno difference\n", real->path);
		return;
	}

	msg(MSG_QUIET, "%s:\t", real->path);
	if (cmp & DIFF_OWNER)
		msg(MSG_QUIET, "owner ");
	if (cmp & DIFF_GROUP)
		msg(MSG_QUIET, "group ");
	if (cmp & DIFF_MODE)
		msg(MSG_QUIET, "mode ");
	if (cmp & DIFF_TYPE)
		msg(MSG_QUIET, "type ");
	if (cmp & DIFF_MTIME)
		msg(MSG_QUIET, "mtime ");
	if (cmp & DIFF_XATTR)
		msg(MSG_QUIET, "xattr ");
	msg(MSG_QUIET, "\n");
}

/*
 * Tries to change the real metadata to match the stored one
 * - for use in mentries_compare
 */
static void
compare_fix(struct metaentry *real, struct metaentry *stored, int cmp)
{
	struct group *group;
	struct passwd *owner;
	gid_t gid = -1;
	uid_t uid = -1;
	struct utimbuf tbuf;
	int i;

	if (!real && !stored) {
		msg(MSG_ERROR, "%s called with incorrect arguments\n",
		    __FUNCTION__);
		return;
	}

	if (!real) {
		msg(MSG_NORMAL, "%s:\tremoved\n", stored->path);
		return;
	}

	if (!stored) {
		msg(MSG_NORMAL, "%s:\tadded\n", real->path);
		return;
	}

	if (cmp == DIFF_NONE) {
		msg(MSG_DEBUG, "%s:\tno difference\n", real->path);
		return;
	}

	if (cmp & DIFF_TYPE) {
		msg(MSG_NORMAL, "%s:\tnew type, will not change metadata\n",
		    real->path);
		return;
	}

	msg(MSG_QUIET, "%s:\tchanging metadata\n", real->path);

	while (cmp & (DIFF_OWNER | DIFF_GROUP)) {
		if (cmp & DIFF_OWNER) {
			msg(MSG_NORMAL, "\tchanging owner from %s to %s\n",
			    real->path, real->group, stored->group);
			owner = getpwnam(stored->owner);
			if (!owner) {
				msg(MSG_DEBUG, "\tgetpwnam failed: %s\n",
				    strerror(errno));
				break;
			}
			uid = owner->pw_uid;
		}

		if (cmp & DIFF_GROUP) {
			msg(MSG_NORMAL, "\tchanging group from %s to %s\n",
			    real->path, real->group, stored->group);
			group = getgrnam(stored->group);
			if (!group) {
				msg(MSG_DEBUG, "\tgetgrnam failed: %s\n",
				    strerror(errno));
				break;
			}
			gid = group->gr_gid;
		}

		if (lchown(real->path, uid, gid)) {
			msg(MSG_DEBUG, "\tlchown failed: %s\n",
			    strerror(errno));
			break;
		}
		break;
	}

	if (cmp & DIFF_MODE) {
		msg(MSG_NORMAL, "%s:\tchanging mode from 0%o to 0%o\n",
		    real->path, real->mode & 07777, stored->mode & 07777);
		if (chmod(real->path, stored->mode & 07777))
			msg(MSG_DEBUG, "\tchmod failed: %s\n", strerror(errno));
	}

	/* FIXME: Use utimensat here, or even better - lutimensat */
	if ((cmp & DIFF_MTIME) && S_ISLNK(real->mode)) {
		msg(MSG_NORMAL, "%s:\tsymlink, not changing mtime", real->path);
	} else if (cmp & DIFF_MTIME) {
		msg(MSG_NORMAL, "%s:\tchanging mtime from %ld to %ld\n",
		    real->path, real->mtime, stored->mtime);
		tbuf.actime = stored->mtime;
		tbuf.modtime = stored->mtime;
		if (utime(real->path, &tbuf)) {
			msg(MSG_DEBUG, "\tutime failed: %s\n", strerror(errno));
			return;
		}
	}

	if (cmp & DIFF_XATTR) {
		for (i = 0; i < real->xattrs; i++) {
			/* Any attrs to remove? */
			if (mentry_find_xattr(stored, real, i) >= 0)
				continue;

			msg(MSG_NORMAL, "%s:\tremoving xattr %s\n",
			    real->path, real->xattr_names[i]);
			if (lremovexattr(real->path, real->xattr_names[i]))
				msg(MSG_DEBUG, "\tlremovexattr failed: %s\n",
				    strerror(errno));
		}

		for (i = 0; i < stored->xattrs; i++) {
			/* Any xattrs to add? (on change they are removed above) */
			if (mentry_find_xattr(real, stored, i) >= 0)
				continue;

			msg(MSG_NORMAL, "%s:\tadding xattr %s\n",
			    stored->path, stored->xattr_names[i]);
			if (lsetxattr(stored->path, stored->xattr_names[i], 
				      stored->xattr_values[i],
				      stored->xattr_lvalues[i], XATTR_CREATE))
				msg(MSG_DEBUG, "\tlsetxattr failed: %s\n",
				    strerror(errno));
		}
	}
}

/* Prints usage message and exits */
static void
usage(const char *arg0, const char *message)
{
	if (message)
		msg(MSG_CRITICAL, "%s: %s\n\n", arg0, msg);
	msg(MSG_CRITICAL, "Usage: %s ACTION [OPTION...] [PATH...]\n\n", arg0);
	msg(MSG_CRITICAL, "Where ACTION is one of:\n"
	    "  -c, --compare\tShow differences between stored and real metadata\n"
	    "  -s, --save\tSave current metadata\n"
	    "  -a, --apply\tApply stored metadata\n"
	    "  -h, --help\tHelp message (this text)\n\n"
	    "Valid OPTIONS are (can be given more than once):\n"
	    "  -v, --verbose\tPrint more verbose messages\n"
	    "  -q, --quiet\tPrint less verbose messages\n"
	    "  -m, --mtime\tAlso take mtime into account for diff or apply\n");

	exit(message ? EXIT_FAILURE : EXIT_SUCCESS);
}

/* Options */
static struct option long_options[] = {
	{"compare", 0, 0, 0},
	{"save", 0, 0, 0},
	{"apply", 0, 0, 0},
	{"help", 0, 0, 0},
	{"verbose", 0, 0, 0},
	{"quiet", 0, 0, 0},
	{"mtime", 0, 0, 0},
	{0, 0, 0, 0}
};

/* Main function */
int
main(int argc, char **argv, char **envp)
{
	int i, c;
	struct metaentry *lreal = NULL;
	struct metaentry *lstored = NULL;
	int action = 0;

	/* Parse options */
	i = 0;
	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "csahvqm",
				long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 0:
			if (!strcmp("verbose",
				    long_options[option_index].name)) {
				adjust_verbosity(1);
			} else if (!strcmp("quiet",
					   long_options[option_index].name)) {
				adjust_verbosity(-1);
			} else if (!strcmp("mtime",
					   long_options[option_index].name)) {
				do_mtime = 1;
			} else {
				action |= (1 << option_index);
				i++;
			}
			break;
		case 'c':
			action |= ACTION_DIFF;
			i++;
			break;
		case 's':
			action |= ACTION_SAVE;
			i++;
			break;
		case 'a':
			action |= ACTION_APPLY;
			i++;
			break;
		case 'h':
			action |= ACTION_HELP;
			i++;
			break;
		case 'v':
			adjust_verbosity(1);
			break;
		case 'q':
			adjust_verbosity(-1);
			break;
		case 'm':
			do_mtime = 1;
			break;
		default:
			usage(argv[0], "unknown option");
		}
	}

	/* Make sure only one action is specified */
	if (i != 1)
		usage(argv[0], "incorrect option(s)");

	/* Perform action */
	switch (action) {
	case ACTION_DIFF:
		mentries_fromfile(&lstored, METAFILE);
		if (!lstored) {
			msg(MSG_CRITICAL, "Failed to load metadata from %s\n",
			    METAFILE);
			exit(EXIT_FAILURE);
		}

		if (optind < argc) {
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &lreal);
		} else {
			mentries_recurse_path(".", &lreal);
		}

		if (!lreal) {
			msg(MSG_CRITICAL,
			    "Failed to load metadata from file system\n");
			exit(EXIT_FAILURE);
		}

		mentries_compare(lreal, lstored, compare_print, do_mtime);
		break;

	case ACTION_SAVE:
		if (optind < argc) {
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &lreal);
		} else {
			mentries_recurse_path(".", &lreal);
		}

		if (!lreal) {
			msg(MSG_CRITICAL,
			    "Failed to load metadata from file system\n");
			exit(EXIT_FAILURE);
		}

		mentries_tofile(lreal, METAFILE);
		break;

	case ACTION_APPLY:
		mentries_fromfile(&lstored, METAFILE);
		if (!lstored) {
			msg(MSG_CRITICAL, "Failed to load metadata from %s\n",
			    METAFILE);
			exit(EXIT_FAILURE);
		}

		if (optind < argc) {
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &lreal);
		} else {
			mentries_recurse_path(".", &lreal);
		}

		if (!lreal) {
			msg(MSG_CRITICAL,
			    "Failed to load metadata from file system\n");
			exit(EXIT_FAILURE);
		}

		mentries_compare(lreal, lstored, compare_fix, do_mtime);
		break;

	case ACTION_HELP:
		usage(argv[0], NULL);
	}

	exit(EXIT_SUCCESS);
}

