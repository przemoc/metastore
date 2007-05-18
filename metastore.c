#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <attr/xattr.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <dirent.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <getopt.h>
#include <stdarg.h>
#include <utime.h>

#include "metastore.h"
#include "utils.h"

int verbosity = 0;
int do_mtime = 0;

int
msg(int level, const char *fmt, ...)
{
	int ret;
	va_list ap;

	if (level > verbosity)
		return 0;

	va_start(ap, fmt);
	ret = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return ret;
}

void
mentry_free(struct metaentry *m)
{
	int i;

	if (!m)
		return;

	free(m->path);
	free(m->owner);
	free(m->group);

	for (i = 0; i < m->xattrs; i++) {
		free(m->xattr_names[i]);
		free(m->xattr_values[i]);
	}

	free(m->xattr_names);
	free(m->xattr_values);
	free(m->xattr_lvalues);

	free(m);
}

struct metaentry *
mentry_alloc()
{
	struct metaentry *mentry;
	mentry = xmalloc(sizeof(struct metaentry));
	memset(mentry, 0, sizeof(struct metaentry));
	return mentry;
}

void
mentry_insert(struct metaentry *mentry, struct metaentry **mhead)
{
	struct metaentry *prev;
	struct metaentry *curr;
	int comp;

	if (!(*mhead)) {
		*mhead = mentry;
		return;
	}

	if (strcmp(mentry->path, (*mhead)->path) < 0) {
		mentry->next = *mhead;
		*mhead = mentry;
		return;
	}

	prev = *mhead;
	for (curr = prev->next; curr; curr = curr->next) {
		comp = strcmp(mentry->path, curr->path);
		if (!comp)
			/* Two matching paths */
			return;
		if (comp < 0)
			break;
		prev = curr;
	}

	if (curr)
		mentry->next = curr;
	prev->next = mentry;
}

void
mentry_print(const struct metaentry *mentry)
{
	int i;

	if (!mentry || !mentry->path) {
		fprintf(stderr, "Incorrect meta entry passed to printmetaentry\n");
		return;
	}

	printf("===========================\n");
	printf("Dump of metaentry %p\n", mentry);
	printf("===========================\n");

	printf("path\t\t: %s\n", mentry->path);
	printf("owner\t\t: %s\n", mentry->owner);
	printf("group\t\t: %s\n", mentry->group);
	printf("mtime\t\t: %ld\n", (unsigned long)mentry->mtime);
	printf("mtimensec\t: %ld\n", (unsigned long)mentry->mtimensec);
	printf("mode\t\t: %ld\n", (unsigned long)mentry->mode);
	for (i = 0; i < mentry->xattrs; i++) {
		printf("xattr[%i]\t: %s=\"", i, mentry->xattr_names[i]);
		binary_print(mentry->xattr_values[i], mentry->xattr_lvalues[i]);
		printf("\"\n");
	}

	printf("===========================\n\n");
}

void
mentries_print(const struct metaentry *mhead)
{
	const struct metaentry *mentry;
	int i;

	for (mentry = mhead; mentry; mentry = mentry->next) {
		i++;
		mentry_print(mentry);
	}

	printf("%i entries in total\n", i);
}

struct metaentry *
mentry_create(const char *path)
{
	ssize_t lsize, vsize;
	char *list, *attr;
	struct stat sbuf;
	struct passwd *pbuf;
	struct group *gbuf;
	int i;
	struct metaentry *mentry;

	if (lstat(path, &sbuf)) {
		perror("lstat");
		return NULL;
	}

	pbuf = getpwuid(sbuf.st_uid);
	if (!pbuf) {
		perror("getpwuid");
		return NULL;
	}

	gbuf = getgrgid(sbuf.st_gid);
	if (!gbuf) {
		perror("getgrgid");
		return NULL;
	}

	mentry = mentry_alloc();
	mentry->path = xstrdup(path);
	mentry->owner = xstrdup(pbuf->pw_name);
	mentry->group = xstrdup(gbuf->gr_name);
	mentry->mode = sbuf.st_mode & 0177777;
	mentry->mtime = sbuf.st_mtim.tv_sec;
	mentry->mtimensec = sbuf.st_mtim.tv_nsec;

	/* symlinks have no xattrs */
	if (S_ISLNK(mentry->mode))
		return mentry;
	
	lsize = listxattr(path, NULL, 0);
	if (lsize < 0) {
		perror("listxattr");
		return NULL;
	}

	list = xmalloc(lsize);
	lsize = listxattr(path, list, lsize);
	if (lsize < 0) {
		perror("listxattr");
		return NULL;
	}

	i = 0;
	for (attr = list; attr < list + lsize; attr = strchr(attr, '\0') + 1) {
		if (*attr == '\0')
			continue;
		i++;
	}

	if (i == 0)
		return mentry;

	mentry->xattrs = i;
	mentry->xattr_names = xmalloc(i * sizeof(char *));
	mentry->xattr_values = xmalloc(i * sizeof(char *));
	mentry->xattr_lvalues = xmalloc(i * sizeof(ssize_t));

	i = 0;
	for (attr = list; attr < list + lsize; attr = strchr(attr, '\0') + 1) {
		if (*attr == '\0')
			continue;

		mentry->xattr_names[i] = xstrdup(attr);
		vsize = getxattr(path, attr, NULL, 0);
		if (vsize < 0) {
			perror("getxattr");
			return NULL;
		}

		mentry->xattr_lvalues[i] = vsize;
		mentry->xattr_values[i] = xmalloc(vsize);

		vsize = getxattr(path, attr, mentry->xattr_values[i], vsize);
		if (vsize < 0) {
			perror("getxattr");
			return NULL;
		}
		i++;
	}

	return mentry;
}

char *
normalize_path(const char *orig)
{
	char *real = canonicalize_file_name(orig);
	char cwd[PATH_MAX];
	char *result;

	getcwd(cwd, PATH_MAX);
	if (!real)
		return NULL;

	if (!strncmp(real, cwd, strlen(cwd))) {
		result = xmalloc(strlen(real) - strlen(cwd) + 1 + 1);
		result[0] = '\0';
		strcat(result, ".");
		strcat(result, real + strlen(cwd));
	} else {
		result = xstrdup(real);
	}

	free(real);
	return result;
}

void
mentries_recurse(const char *path, struct metaentry **mhead)
{
	struct stat sbuf;
	struct metaentry *mentry;
	char tpath[PATH_MAX];
	DIR *dir;
	struct dirent *dent;

	if (!path)
		return;

	if (lstat(path, &sbuf)) {
		printf("Failed to stat %s\n", path);
		return;
	}

	mentry = mentry_create(path);
	if (!mentry) {
		printf("Failed to get metadata for %s\n", path);
		return;
	}

	mentry_insert(mentry, mhead);

	if (S_ISDIR(sbuf.st_mode)) {
		dir = opendir(path);
		if (!dir) {
			printf("Failed to open dir %s\n", path);
			return;
		}

		while ((dent = readdir(dir))) {
			if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..") || !strcmp(dent->d_name, ".git"))
				continue;
			snprintf(tpath, PATH_MAX, "%s/%s", path, dent->d_name);
			tpath[PATH_MAX - 1] = '\0';
			mentries_recurse(tpath, mhead);
		}

		closedir(dir);
	}
}

void
mentries_recurse_path(const char *opath, struct metaentry **mhead)
{
	char *path = normalize_path(opath);
	mentries_recurse(path, mhead);
	free(path);
}

void 
mentries_tofile(const struct metaentry *mhead, const char *path)
{
	FILE *to;
	const struct metaentry *mentry;
	int i;

	to = fopen(path, "w");
	if (!to) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	write_binary_string(SIGNATURE, SIGNATURELEN, to);
	write_binary_string(VERSION, VERSIONLEN, to);

	for (mentry = mhead; mentry; mentry = mentry->next) {
		write_string(mentry->path, to);
		write_string(mentry->owner, to);
		write_string(mentry->group, to);
		write_int((uint64_t)mentry->mtime, 8, to);
		write_int((uint64_t)mentry->mtimensec, 8, to);
		write_int((uint64_t)mentry->mode, 2, to);
		write_int(mentry->xattrs, 4, to);
		for (i = 0; i < mentry->xattrs; i++) {
			write_string(mentry->xattr_names[i], to);
			write_int(mentry->xattr_lvalues[i], 4, to);
			write_binary_string(mentry->xattr_values[i], mentry->xattr_lvalues[i], to);
		}
	}

	fclose(to);
}

void
mentries_fromfile(struct metaentry **mhead, const char *path)
{
	struct metaentry *mentry;
	char *mmapstart;
	char *ptr;
	char *max;
	int fd;
	struct stat sbuf;
	int i;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	if (fstat(fd, &sbuf)) {
		perror("fstat");
		exit(EXIT_FAILURE);
	}

	if (sbuf.st_size < (SIGNATURELEN + VERSIONLEN)) {
		fprintf(stderr, "Invalid size for file %s\n", path);
		exit(EXIT_FAILURE);
	}

	mmapstart = mmap(NULL, (size_t)sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (mmapstart == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	ptr = mmapstart;
	max = mmapstart + sbuf.st_size;

	if (strncmp(ptr, SIGNATURE, SIGNATURELEN)) {
		printf("Invalid signature for file %s\n", path);
		goto out;
	}
	ptr += SIGNATURELEN;

	if (strncmp(ptr, VERSION, VERSIONLEN)) {
		printf("Invalid version for file %s\n", path);
		goto out;
	}
	ptr += VERSIONLEN;

	while (ptr < mmapstart + sbuf.st_size) {
		if (*ptr == '\0') {
			fprintf(stderr, "Invalid characters in file %s\n", path);
			goto out;
		}

		mentry = mentry_alloc();
		mentry->path = read_string(&ptr, max);
		mentry->owner = read_string(&ptr, max);
		mentry->group = read_string(&ptr, max);
		mentry->mtime = (time_t)read_int(&ptr, 8, max);
		mentry->mtimensec = (time_t)read_int(&ptr, 8, max);
		mentry->mode = (mode_t)read_int(&ptr, 2, max);
		mentry->xattrs = (unsigned int)read_int(&ptr, 4, max);

		if (mentry->xattrs > 0) {
			mentry->xattr_names = xmalloc(mentry->xattrs * sizeof(char *));
			mentry->xattr_lvalues = xmalloc(mentry->xattrs * sizeof(int));
			mentry->xattr_values = xmalloc(mentry->xattrs * sizeof(char *));

			for (i = 0; i < mentry->xattrs; i++) {
				mentry->xattr_names[i] = read_string(&ptr, max);
				mentry->xattr_lvalues[i] = (int)read_int(&ptr, 4, max);
				mentry->xattr_values[i] = read_binary_string(&ptr, mentry->xattr_lvalues[i], max);
			}
		}
		mentry_insert(mentry, mhead);
	}

out:
	munmap(mmapstart, sbuf.st_size);
	close(fd);
}

struct metaentry *
mentry_find(const char *path, struct metaentry *mhead)
{
	struct metaentry *m;

	/* FIXME - We can do a bisect search here instead */
	for (m = mhead; m; m = m->next) {
		if (!strcmp(path, m->path))
			return m;
	}
	return NULL;
}

/* Returns xattr index in haystack which corresponds to xattr n in needle */
int
mentry_find_xattr(struct metaentry *haystack, struct metaentry *needle, int n)
{
	int i;

	for (i = 0; i < haystack->xattrs; i++) {
		if (strcmp(haystack->xattr_names[i], needle->xattr_names[n]))
			continue;
		if (haystack->xattr_lvalues[i] != needle->xattr_lvalues[n])
			return -1;
		if (bcmp(haystack->xattr_values[i], needle->xattr_values[n], needle->xattr_lvalues[n]))
			return -1;
		return i;
	}
	return -1;
}

/* Returns zero if all xattrs in left and right match */
int
mentry_compare_xattr(struct metaentry *left, struct metaentry *right)
{
	int i;

	if (left->xattrs != right->xattrs)
		return 1;

	/* Make sure all xattrs in left are found in right and vice versa */
	for (i = 0; i < left->xattrs; i++) {
		if (mentry_find_xattr(right, left, i) < 0 ||
		    mentry_find_xattr(left, right, i) < 0) {
			return 1;
		}
	}

	return 0;
}

int
mentry_compare(struct metaentry *left, struct metaentry *right)
{
	int retval = DIFF_NONE;

	if (!left || !right) {
		fprintf(stderr, "mentry_compare called with empty arguments\n");
		return -1;
	}

	if (strcmp(left->path, right->path))
		return -1;

	if (strcmp(left->owner, right->owner))
		retval |= DIFF_OWNER;

	if (strcmp(left->group, right->group))
		retval |= DIFF_GROUP;

	if ((left->mode & 07777) != (right->mode & 07777))
		retval |= DIFF_MODE;

	if ((left->mode & S_IFMT) != (right->mode & S_IFMT))
		retval |= DIFF_TYPE;

	if (do_mtime && strcmp(left->path, METAFILE) && 
	    (left->mtime != right->mtime || left->mtimensec != right->mtimensec))
		retval |= DIFF_MTIME;

	if (mentry_compare_xattr(left, right)) {
		retval |= DIFF_XATTR;
		return retval;
	}

	return retval;
}

void
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

void
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

void
mentries_compare(struct metaentry *mheadleft,
		 struct metaentry *mheadright,
		 void (*printfunc)(struct metaentry *, struct metaentry *, int))
{
	struct metaentry *left, *right;
	int cmp;

	if (!mheadleft || !mheadright) {
		fprintf(stderr, "mentries_compare called with empty list\n");
		return;
	}

	for (left = mheadleft; left; left = left->next) {
		right = mentry_find(left->path, mheadright);
		if (!right)
			cmp = DIFF_ADDED;
		else
			cmp = mentry_compare(left, right);
		printfunc(left, right, cmp);
	}

	for (right = mheadright; right; right = right->next) {
		left = mentry_find(right->path, mheadleft);
		if (!left)
			printfunc(left, right, DIFF_DELE);
	}
}

void
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

	if (msg)
		exit(EXIT_FAILURE);
	exit(EXIT_SUCCESS);
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
