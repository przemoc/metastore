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

#define _BSD_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <utime.h>
#include <attr/xattr.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "metastore.h"
#include "utils.h"
#include "metaentry.h"

/* Used to store the path to the file containing the metadata */
char *metafile = METAFILE;

/* Used to indicate whether mtimes should be corrected */
static bool do_mtime = false;

/* Used to indicate whether empty dirs should be recreated */
static bool do_emptydirs = false;

/* Used to indicate whether .git dirs should be processed */
static bool do_git = false;

/* Used to indicate whether NTFS system attributes should be stored */
static bool do_ntfs = false;

/* Used to create lists of dirs / other files which are missing in the fs */
static struct metaentry *missingdirs = NULL;
static struct metaentry *missingothers = NULL;

/*
 * Inserts an entry in a linked list ordered by pathlen 
 */
static void
insert_entry_plist(struct metaentry **list, struct metaentry *entry)
{
	struct metaentry **parent;

	for (parent = list; *parent; parent = &((*parent)->list)) {
		if ((*parent)->pathlen > entry->pathlen)
			break;
	}

	entry->list = *parent;
	*parent = entry;
}
	
/*
 * Prints differences between real and stored actual metadata
 * - for use in mentries_compare
 */
static void
compare_print(struct metaentry *real, struct metaentry *stored, int cmp)
{
	if (!real && !stored) {
		msg(MSG_ERROR, "%s called with incorrect arguments\n", __FUNCTION__);
		return;
	}

	if (cmp == DIFF_NONE) {
		msg(MSG_DEBUG, "%s:\tno difference\n", real->path);
		return;
	}

	msg(MSG_QUIET, "%s:\t", real ? real->path : stored->path);

	if (cmp & DIFF_ADDED)
		msg(MSG_QUIET, "added ", real->path);
	if (cmp & DIFF_DELE)
		msg(MSG_QUIET, "removed ", stored->path);
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
		if (S_ISDIR(stored->mode))
			insert_entry_plist(&missingdirs, stored);
		else
			insert_entry_plist(&missingothers, stored);

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
			msg(MSG_NORMAL, "%s:\tchanging owner from %s to %s\n",
			    real->path, real->group, stored->group);
			owner = xgetpwnam(stored->owner);
			if (!owner) {
				msg(MSG_DEBUG, "\tgetpwnam failed: %s\n",
				    strerror(errno));
				break;
			}
			uid = owner->pw_uid;
		}

		if (cmp & DIFF_GROUP) {
			msg(MSG_NORMAL, "%s:\tchanging group from %s to %s\n",
			    real->path, real->group, stored->group);
			group = xgetgrnam(stored->group);
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
		msg(MSG_NORMAL, "%s:\tsymlink, not changing mtime\n", real->path);
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

/*
 * Tries to fix any empty dirs which are missing by recreating them.
 * An "empty" dir is one which either:
 *  - is empty; or
 *  - only contained empty dirs
 */
static void
fixup_emptydirs(struct metahash *real, struct metahash *stored)
{
	struct metaentry *entry;
	struct metaentry *cur;
	struct metaentry **parent;
	char *bpath;
	char *delim;
	size_t blen;
	struct metaentry *new;

	if (!missingdirs)
		return;
	msg(MSG_DEBUG, "\nAttempting to recreate missing dirs\n");

	/* If directory x/y is missing, but file x/y/z is also missing,
	 * we should prune directory x/y from the list of directories to
	 * recreate since the deletition of x/y is likely to be genuine
	 * (as opposed to empty dir pruning like git/cvs does).
	 *
	 * Also, if file x/y/z is missing, any child directories of
	 * x/y should be pruned as they are probably also intentionally
	 * removed.
	 */

	msg(MSG_DEBUG, "List of candidate dirs:\n");
	for (cur = missingdirs; cur; cur = cur->list)
		msg(MSG_DEBUG, " %s\n", cur->path);

	for (entry = missingothers; entry; entry = entry->list) {
		msg(MSG_DEBUG, "Pruning using file %s\n", entry->path);
		bpath = xstrdup(entry->path);
		delim = strrchr(bpath, '/');
		if (!delim) {
			msg(MSG_NORMAL, "No delimiter found in %s\n", bpath);
			free(bpath);
			continue;
		}
		*delim = '\0';

		parent = &missingdirs;
		for (cur = *parent; cur; cur = cur->list) {
			if (strcmp(cur->path, bpath)) {
				parent = &cur->list;
				continue;
			}

			msg(MSG_DEBUG, "Prune phase 1 - %s\n", cur->path);
			*parent = cur->list;
		}

		/* Now also prune subdirs of the base dir */
		*delim++ = '/';
		*delim = '\0';
		blen = strlen(bpath);

		parent = &missingdirs;
		for (cur = *parent; cur; cur = cur->list) {
			if (strncmp(cur->path, bpath, blen)) {
				parent = &cur->list;
				continue;
			}

			msg(MSG_DEBUG, "Prune phase 2 - %s\n", cur->path);
			*parent = cur->list;
		}

		free(bpath);
	}
	msg(MSG_DEBUG, "\n");

	for (cur = missingdirs; cur; cur = cur->list) {
		msg(MSG_QUIET, "%s:\trecreating...", cur->path);
		if (mkdir(cur->path, cur->mode)) {
			msg(MSG_QUIET, "failed (%s)\n", strerror(errno));
			continue;
		}
		msg(MSG_QUIET, "ok\n");

		new = mentry_create(cur->path, do_ntfs);
		if (!new) {
			msg(MSG_QUIET, "Failed to get metadata for %s\n");
			continue;
		}

		compare_fix(new, cur, mentry_compare(new, cur, do_mtime));
	}
}

/* Prints usage message and exits */
static void
usage(const char *arg0, const char *message)
{
	if (message)
		msg(MSG_CRITICAL, "%s: %s\n\n", arg0, message);
	msg(MSG_CRITICAL, "Usage: %s ACTION [OPTION...] [PATH...]\n\n", arg0);
	msg(MSG_CRITICAL, "Where ACTION is one of:\n"
	    "  -c, --compare\t\tShow differences between stored and real metadata\n"
	    "  -s, --save\t\tSave current metadata\n"
	    "  -a, --apply\t\tApply stored metadata\n"
	    "  -h, --help\t\tHelp message (this text)\n\n"
	    "Valid OPTIONS are:\n"
	    "  -v, --verbose\t\tPrint more verbose messages\n"
	    "  -q, --quiet\t\tPrint less verbose messages\n"
	    "  -m, --mtime\t\tAlso take mtime into account for diff or apply\n"
	    "  -e, --empty-dirs\tRecreate missing empty directories (experimental)\n"
	    "  -g, --git\t\tDo not omit .git directories\n"
	    "  -f, --file   <file>\tSet metadata file\n"
            "  -n, --ntfs\t\tSearch for ntfs system attributes\n"
	    );

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
	{"empty-dirs", 0, 0, 0},
	{"git", 0, 0, 0},
	{"file", required_argument, 0, 0},
        {"ntfs", 0, 0, 0},
	{0, 0, 0, 0}
};

/* Main function */
int
main(int argc, char **argv, char **envp)
{
	int i, c;
	struct metahash *real = NULL;
	struct metahash *stored = NULL;
	int action = 0;

	/* Parse options */
	i = 0;
	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "csahvqmegf:n",
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
				do_mtime = true;
			} else if (!strcmp("empty-dirs",
					   long_options[option_index].name)) {
				do_emptydirs = true;
			} else if (!strcmp("git",
					   long_options[option_index].name)) {
				do_git = true;
			} else if (!strcmp("file",
				   long_options[option_index].name)) {
				metafile = optarg;
			} else if (!strcmp("ntfs",
					   long_options[option_index].name)) {
				do_ntfs = true;
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
			do_mtime = true;
			break;
		case 'e':
			do_emptydirs = true;
			break;
		case 'g':
			do_git = true;
			break;
		case 'f':
			metafile = optarg;
			break;
                case 'n':
                        do_ntfs = true;
                        break;
		default:
			usage(argv[0], "unknown option");
		}
	}

	/* Make sure only one action is specified */
	if (i != 1)
		usage(argv[0], "incorrect option(s)");

	/* Make sure --empty-dirs is only used with apply */
	if (do_emptydirs && action != ACTION_APPLY)
		usage(argv[0], "--empty-dirs is only valid with --apply");

	/* Perform action */
	switch (action) {
	case ACTION_DIFF:
		mentries_fromfile(&stored, metafile);
		if (!stored) {
			msg(MSG_CRITICAL, "Failed to load metadata from %s\n",
			    metafile);
			exit(EXIT_FAILURE);
		}

		if (optind < argc) {
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &real, do_git, do_ntfs);
		} else {
			mentries_recurse_path(".", &real, do_git, do_ntfs);
		}

		if (!real) {
			msg(MSG_CRITICAL,
			    "Failed to load metadata from file system\n");
			exit(EXIT_FAILURE);
		}

		mentries_compare(real, stored, compare_print, do_mtime);
		break;

	case ACTION_SAVE:
		if (optind < argc) {
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &real, do_git, do_ntfs);
		} else {
			mentries_recurse_path(".", &real, do_git, do_ntfs);
		}

		if (!real) {
			msg(MSG_CRITICAL,
			    "Failed to load metadata from file system\n");
			exit(EXIT_FAILURE);
		}

		mentries_tofile(real, metafile);
		break;

	case ACTION_APPLY:
		mentries_fromfile(&stored, metafile);
		if (!stored) {
			msg(MSG_CRITICAL, "Failed to load metadata from %s\n",
			    metafile);
			exit(EXIT_FAILURE);
		}

		if (optind < argc) {
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &real, do_git, do_ntfs);
		} else {
			mentries_recurse_path(".", &real, do_git, do_ntfs);
		}

		if (!real) {
			msg(MSG_CRITICAL,
			    "Failed to load metadata from file system\n");
			exit(EXIT_FAILURE);
		}

		mentries_compare(real, stored, compare_fix, do_mtime);

		if (do_emptydirs)
			fixup_emptydirs(real, stored);
		break;

	case ACTION_HELP:
		usage(argv[0], NULL);
	}

	exit(EXIT_SUCCESS);
}

