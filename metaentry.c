#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <attr/xattr.h>
#include <limits.h>
#include <dirent.h>
#include <sys/mman.h>
#include <utime.h>
#include <fcntl.h>
#include <stdint.h>

#include "metastore.h"
#include "metaentry.h"
#include "utils.h"

#if 0
static void
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
#endif

static struct metaentry *
mentry_alloc()
{
	struct metaentry *mentry;
	mentry = xmalloc(sizeof(struct metaentry));
	memset(mentry, 0, sizeof(struct metaentry));
	return mentry;
}

static void
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

#ifdef DEBUG
static void
mentry_print(const struct metaentry *mentry)
{
	int i;

	if (!mentry || !mentry->path) {
		msg(MSG_DEBUG, "Incorrect meta entry passed to printmetaentry\n");
		return;
	}

	msg(MSG_DEBUG, "===========================\n");
	msg(MSG_DEBUG, "Dump of metaentry %p\n", mentry);
	msg(MSG_DEBUG, "===========================\n");

	msg(MSG_DEBUG, "path\t\t: %s\n", mentry->path);
	msg(MSG_DEBUG, "owner\t\t: %s\n", mentry->owner);
	msg(MSG_DEBUG, "group\t\t: %s\n", mentry->group);
	msg(MSG_DEBUG, "mtime\t\t: %ld\n", (unsigned long)mentry->mtime);
	msg(MSG_DEBUG, "mtimensec\t: %ld\n", (unsigned long)mentry->mtimensec);
	msg(MSG_DEBUG, "mode\t\t: %ld\n", (unsigned long)mentry->mode);
	for (i = 0; i < mentry->xattrs; i++) {
		msg(MSG_DEBUG, "xattr[%i]\t: %s=\"", i, mentry->xattr_names[i]);
		binary_print(mentry->xattr_values[i], mentry->xattr_lvalues[i]);
		msg(MSG_DEBUG, "\"\n");
	}

	msg(MSG_DEBUG, "===========================\n\n");
}

static void
mentries_print(const struct metaentry *mhead)
{
	const struct metaentry *mentry;
	int i;

	for (mentry = mhead; mentry; mentry = mentry->next) {
		i++;
		mentry_print(mentry);
	}

	msg(MSG_DEBUG, "%i entries in total\n", i);
}
#endif

static struct metaentry *
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
		msg(MSG_ERROR, "lstat failed for %s: %s\n", path, strerror(errno));
		return NULL;
	}

	pbuf = getpwuid(sbuf.st_uid);
	if (!pbuf) {
		msg(MSG_ERROR, "getpwuid failed for %i: %s\n", (int)sbuf.st_uid, strerror(errno));
		return NULL;
	}

	gbuf = getgrgid(sbuf.st_gid);
	if (!gbuf) {
		msg(MSG_ERROR, "getgrgid failed for %i: %s\n", (int)sbuf.st_gid, strerror(errno));
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
		msg(MSG_ERROR, "listxattr failed for %s: %s\n", path, strerror(errno));
		return NULL;
	}

	list = xmalloc(lsize);
	lsize = listxattr(path, list, lsize);
	if (lsize < 0) {
		msg(MSG_ERROR, "listxattr failed for %s: %s\n", path, strerror(errno));
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
			msg(MSG_ERROR, "getxattr failed for %s: %s\n", path, strerror(errno));
			return NULL;
		}

		mentry->xattr_lvalues[i] = vsize;
		mentry->xattr_values[i] = xmalloc(vsize);

		vsize = getxattr(path, attr, mentry->xattr_values[i], vsize);
		if (vsize < 0) {
			msg(MSG_ERROR, "getxattr failed for %s: %s\n", path, strerror(errno));
			return NULL;
		}
		i++;
	}

	return mentry;
}

static char *
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

static void
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
		msg(MSG_ERROR, "lstat failed for %s: %s\n", path, strerror(errno));
		return;
	}

	mentry = mentry_create(path);
	if (!mentry)
		return;

	mentry_insert(mentry, mhead);

	if (S_ISDIR(sbuf.st_mode)) {
		dir = opendir(path);
		if (!dir) {
			msg(MSG_ERROR, "opendir failed for %s: %s\n", path, strerror(errno));
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
		msg(MSG_CRITICAL, "File %s has an invalid size\n", path);
		exit(EXIT_FAILURE);
	}

	mmapstart = mmap(NULL, (size_t)sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (mmapstart == MAP_FAILED) {
		msg(MSG_CRITICAL, "Unable to mmap %s: %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}
	ptr = mmapstart;
	max = mmapstart + sbuf.st_size;

	if (strncmp(ptr, SIGNATURE, SIGNATURELEN)) {
		msg(MSG_CRITICAL, "Invalid signature for file %s\n", path);
		goto out;
	}
	ptr += SIGNATURELEN;

	if (strncmp(ptr, VERSION, VERSIONLEN)) {
		msg(MSG_CRITICAL, "Invalid version of file %s\n", path);
		goto out;
	}
	ptr += VERSIONLEN;

	while (ptr < mmapstart + sbuf.st_size) {
		if (*ptr == '\0') {
			msg(MSG_CRITICAL, "Invalid characters in file %s\n", path);
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

static struct metaentry *
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
static int
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

static int
mentry_compare(struct metaentry *left, struct metaentry *right)
{
	int retval = DIFF_NONE;

	if (!left || !right) {
		msg(MSG_ERROR, "%s called with empty list\n", __FUNCTION__);
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
mentries_compare(struct metaentry *mheadleft,
		 struct metaentry *mheadright,
		 void (*printfunc)(struct metaentry *, struct metaentry *, int))
{
	struct metaentry *left, *right;
	int cmp;

	if (!mheadleft || !mheadright) {
		msg(MSG_ERROR, "%s called with empty list\n", __FUNCTION__);
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

