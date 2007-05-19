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

int do_mtime = 0;

static void
compare_print(struct metaentry *left, struct metaentry *right, int cmp)
{
	if (!left) {
		msg(MSG_QUIET, "%s:\tremoved\n", right->path);
		return;
	}

	if (!right) {
		msg(MSG_QUIET, "%s:\tadded\n", left->path);
		return;
	}

	if (cmp == DIFF_NONE) {
		msg(MSG_DEBUG, "%s:\tno difference\n", left->path);
		return;
	}

	msg(MSG_QUIET, "%s:\t", left->path);
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

static void
compare_fix(struct metaentry *left, struct metaentry *right, int cmp)
{
	struct group *group;
	struct passwd *owner;
	gid_t gid = -1;
	uid_t uid = -1;
	struct utimbuf tbuf;
	int i;

	if (!left && !right) {
		msg(MSG_ERROR, "%s called with incorrect arguments\n", __FUNCTION__);
		return;
	}

	if (!left) {
		msg(MSG_NORMAL, "%s:\tremoved\n", right->path);
		return;
	}

	if (!right) {
		msg(MSG_NORMAL, "%s:\tadded\n", left->path);
		return;
	}

	if (cmp == DIFF_NONE) {
		msg(MSG_DEBUG, "%s:\tno difference\n", left->path);
		return;
	}

	if (cmp & DIFF_TYPE) {
		msg(MSG_NORMAL, "%s:\tnew type, will not change metadata\n", left->path);
		return;
	}

	msg(MSG_QUIET, "%s:\tchanging metadata\n", left->path);

	while (cmp & (DIFF_OWNER | DIFF_GROUP)) {
		if (cmp & DIFF_OWNER) {
			msg(MSG_NORMAL, "\tchanging owner from %s to %s\n", left->path, left->group, right->group);
			owner = getpwnam(right->owner);
			if (!owner) {
				msg(MSG_DEBUG, "\tgetpwnam failed: %s\n", strerror(errno));
				break;
			}
			uid = owner->pw_uid;
		}

		if (cmp & DIFF_GROUP) {
			msg(MSG_NORMAL, "\tchanging group from %s to %s\n", left->path, left->group, right->group);
			group = getgrnam(right->group);
			if (!group) {
				msg(MSG_DEBUG, "\tgetgrnam failed: %s\n", strerror(errno));
				break;
			}
			gid = group->gr_gid;
		}

		if (lchown(left->path, uid, gid)) {
			msg(MSG_DEBUG, "\tlchown failed: %s\n", strerror(errno));
			break;
		}
		break;
	}

	if (cmp & DIFF_MODE) {
		msg(MSG_NORMAL, "%s:\tchanging mode from 0%o to 0%o\n", left->path, left->mode, right->mode);
		if (chmod(left->path, left->mode))
			msg(MSG_DEBUG, "\tchmod failed: %s\n", strerror(errno));
	}

	if (cmp & DIFF_MTIME) {
		msg(MSG_NORMAL, "%s:\tchanging mtime from %ld to %ld\n", left->path, left->mtime, right->mtime);
		/* FIXME: Use utimensat here */
		tbuf.actime = right->mtime;
		tbuf.modtime = right->mtime;
		if (utime(left->path, &tbuf)) {
			msg(MSG_DEBUG, "\tutime failed: %s\n", strerror(errno));
			return;
		}
	}

	if (cmp & DIFF_XATTR) {
		for (i = 0; i < left->xattrs; i++) {
			/* Any attrs to remove? */
			if (mentry_find_xattr(right, left, i) >= 0)
				continue;

			msg(MSG_NORMAL, "%s:\tremoving xattr %s\n",
			    left->path, left->xattr_names[i]);
			if (lremovexattr(left->path, left->xattr_names[i]))
				msg(MSG_DEBUG, "\tlremovexattr failed: %s\n", strerror(errno));
		}

		for (i = 0; i < right->xattrs; i++) {
			/* Any xattrs to add? (on change they are removed above) */
			if (mentry_find_xattr(left, right, i) >= 0)
				continue;

			msg(MSG_NORMAL, "%s:\tadding xattr %s\n",
			    right->path, right->xattr_names[i]);
			if (lsetxattr(right->path, right->xattr_names[i], 
				      right->xattr_values[i], right->xattr_lvalues[i], XATTR_CREATE))
				msg(MSG_DEBUG, "\tlsetxattr failed: %s\n", strerror(errno));
		}
	}
}

static void
usage(const char *arg0, const char *message)
{
	if (message)
		msg(MSG_CRITICAL, "%s: %s\n\n", arg0, msg);
	msg(MSG_CRITICAL, "Usage: %s ACTION [OPTIONS] [PATH]...\n\n", arg0);
	msg(MSG_CRITICAL, "Where ACTION is one of:\n"
	    "  -d, --diff\tShow differences between stored and actual metadata\n"
	    "  -s, --save\tSave current metadata\n"
	    "  -a, --apply\tApply stored metadata\n"
	    "  -h, --help\tHelp message (this text)\n\n"
	    "Valid OPTIONS are (can be given more than once):\n"
	    "  -v, --verbose\tPrint more verbose messages\n"
	    "  -q, --quiet\tPrint less verbose messages\n");

	exit(message ? EXIT_FAILURE : EXIT_SUCCESS);
}

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

int
main(int argc, char **argv, char **envp)
{
	int i, c;
	struct metaentry *mhead = NULL;
	struct metaentry *mfhead = NULL;
	int action = 0;

	i = 0;
	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "csahvqm", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 0:
			if (!strcmp("verbose", long_options[option_index].name)) {
				verbosity++;
			} else if (!strcmp("quiet", long_options[option_index].name)) {
				verbosity--;
			} else if (!strcmp("mtime", long_options[option_index].name)) {
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
			verbosity++;
			break;
		case 'q':
			verbosity--;
			break;
		case 'm':
			do_mtime = 1;
			break;
		default:
			usage(argv[0], "unknown option");
		}
	}

	if (i != 1)
		usage(argv[0], "incorrect option(s)");

	switch (action) {
	case ACTION_DIFF:
		mentries_fromfile(&mfhead, METAFILE);
		if (!mfhead) {
			msg(MSG_CRITICAL, "Failed to load metadata from file\n");
			exit(EXIT_FAILURE);
		}

		if (optind < argc) {
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &mhead);
		} else {
			mentries_recurse_path(".", &mhead);
		}

		if (!mhead) {
			msg(MSG_CRITICAL, "Failed to load metadata from file system\n");
			exit(EXIT_FAILURE);
		}

		mentries_compare(mhead, mfhead, compare_print);
		break;

	case ACTION_SAVE:
		if (optind < argc) {
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &mhead);
		} else {
			mentries_recurse_path(".", &mhead);
		}

		if (!mhead) {
			msg(MSG_CRITICAL, "Failed to load metadata from file system\n");
			exit(EXIT_FAILURE);
		}

		mentries_tofile(mhead, METAFILE);
		break;

	case ACTION_APPLY:
		mentries_fromfile(&mfhead, METAFILE);
		if (!mfhead) {
			msg(MSG_CRITICAL, "Failed to load metadata from file\n");
			exit(EXIT_FAILURE);
		}

		if (optind < argc) {
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &mhead);
		} else {
			mentries_recurse_path(".", &mhead);
		}

		if (!mhead) {
			msg(MSG_CRITICAL, "Failed to load metadata from file system\n");
			exit(EXIT_FAILURE);
		}

		mentries_compare(mhead, mfhead, compare_fix);
		break;

	case ACTION_HELP:
		usage(argv[0], NULL);
	}

	exit(EXIT_SUCCESS);
}
