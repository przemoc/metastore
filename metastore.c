#include <stdio.h>
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
		printf("Path %s: removed\n", right->path);
		return;
	}

	if (!right) {
		printf("Path %s: added\n", left->path);
		return;
	}

	if (cmp == DIFF_NONE) {
		msg(MSG_DEBUG, "Path %s: no difference\n", left->path);
		return;
	}

	printf("Path %s: ", left->path);
	if (cmp & DIFF_OWNER)
		printf("owner ");
	if (cmp & DIFF_GROUP)
		printf("group ");
	if (cmp & DIFF_MODE)
		printf("mode ");
	if (cmp & DIFF_TYPE)
		printf("type ");
	if (cmp & DIFF_MTIME)
		printf("mtime ");
	if (cmp & DIFF_XATTR)
		printf("xattr ");
	printf("\n");
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
		printf("%s called with incorrect arguments\n", __FUNCTION__);
		return;
	}

	if (!left) {
		printf("Path %s: removed\n", right->path);
		return;
	}

	if (!right) {
		printf("Path %s: added\n", left->path);
		return;
	}

	if (cmp == DIFF_NONE) {
		msg(MSG_DEBUG, "Path %s: no difference\n", left->path);
		return;
	}

	if (cmp & DIFF_TYPE) {
		printf("Path %s: new type, will not change metadata\n", left->path);
		return;
	}

	if (cmp & (DIFF_OWNER | DIFF_GROUP)) {
		if (cmp & DIFF_OWNER) {
			printf("Path %s: fixing owner from %s to %s\n", left->path, left->group, right->group);
			owner = getpwnam(right->owner);
			if (!owner) {
				perror("getpwnam");
				return;
			}
			uid = owner->pw_uid;
		}

		if (cmp & DIFF_GROUP) {
			printf("Path %s: fixing group from %s to %s\n", left->path, left->group, right->group);
			group = getgrnam(right->group);
			if (!group) {
				perror("getgrnam");
				return;
			}
			gid = group->gr_gid;
		}

		if (lchown(left->path, uid, gid)) {
			perror("lchown");
			return;
		}
		printf("Success\n");
	}

	if (cmp & DIFF_MODE) {
		printf("Path %s: fixing mode from 0%o to 0%o\n", left->path, left->mode, right->mode);
		if (chmod(left->path, left->mode)) {
			perror("chmod");
			return;
		}
	}

	if (cmp & DIFF_MTIME) {
		printf("Path %s: fixing mtime %ld to %ld\n", left->path, left->mtime, right->mtime);
		/* FIXME: Use utimensat here */
		tbuf.actime = right->mtime;
		tbuf.modtime = right->mtime;
		if (utime(left->path, &tbuf)) {
			perror("utime");
			return;
		}
	}

	if (cmp & DIFF_XATTR) {
		for (i = 0; i < left->xattrs; i++) {
			/* Any attrs to remove? */
			if (mentry_find_xattr(right, left, i) >= 0)
				continue;

			msg(MSG_NORMAL, "Path %s: removing xattr %s\n",
			    left->path, left->xattr_names[i]);
			if (lremovexattr(left->path, left->xattr_names[i]))
				perror("lremovexattr");
		}

		for (i = 0; i < right->xattrs; i++) {
			/* Any xattrs to add? (on change they are removed above) */
			if (mentry_find_xattr(left, right, i) >= 0)
				continue;

			msg(MSG_NORMAL, "Path %s: adding xattr %s\n",
			    right->path, right->xattr_names[i]);
			if (lsetxattr(right->path, right->xattr_names[i], 
				       right->xattr_values[i], right->xattr_lvalues[i], XATTR_CREATE))
				perror("lsetxattr");
		}
	}
}

static void
usage(const char *arg0, const char *msg)
{
	if (msg)
		fprintf(stderr, "%s: %s\n\n", arg0, msg);
	fprintf(stderr, "Usage: %s ACTION [OPTIONS] [PATH]...\n\n", arg0);
	fprintf(stderr, "Where ACTION is one of:\n"
			"  -d, --diff\tShow differences between stored and actual metadata\n"
			"  -s, --save\tSave current metadata\n"
			"  -a, --apply\tApply stored metadata\n"
			"  -h, --help\tHelp message (this text)\n\n"
			"Valid OPTIONS are (can be given more than once):\n"
			"  -v, --verbose\tPrint more verbose messages\n"
			"  -q, --quiet\tPrint less verbose messages\n");

	exit(msg ? EXIT_FAILURE : EXIT_SUCCESS);
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
			fprintf(stderr, "Failed to load metadata from file\n");
			exit(EXIT_FAILURE);
		}

		if (optind < argc)
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &mhead);
		else
			mentries_recurse_path(".", &mhead);
		if (!mhead) {
			fprintf(stderr, "Failed to load metadata from fs\n");
			exit(EXIT_FAILURE);
		}
		mentries_compare(mhead, mfhead, compare_print);
		break;
	case ACTION_SAVE:
		if (optind < argc)
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &mhead);
		else
			mentries_recurse_path(".", &mhead);
		if (!mhead) {
			fprintf(stderr, "Failed to load metadata from fs\n");
			exit(EXIT_FAILURE);
		}
		mentries_tofile(mhead, METAFILE);
		break;
	case ACTION_APPLY:
		mentries_fromfile(&mfhead, METAFILE);
		if (!mfhead) {
			fprintf(stderr, "Failed to load metadata from file\n");
			exit(EXIT_FAILURE);
		}

		if (optind < argc)
			while (optind < argc)
				mentries_recurse_path(argv[optind++], &mhead);
		else
			mentries_recurse_path(".", &mhead);
		if (!mhead) {
			fprintf(stderr, "Failed to load metadata from fs\n");
			exit(EXIT_FAILURE);
		}
		mentries_compare(mhead, mfhead, compare_fix);
		break;
	case ACTION_HELP:
		usage(argv[0], NULL);
	}

	exit(EXIT_SUCCESS);
}
