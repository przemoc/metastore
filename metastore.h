#define SIGNATURE    "MeTaSt00r3"
#define SIGNATURELEN 10
#define VERSION      "\0\0\0\0\0\0\0\0"
#define VERSIONLEN   8
#define METAFILE     "./.metadata"

#define ACTION_DIFF  0x01
#define ACTION_SAVE  0x02
#define ACTION_APPLY 0x04
#define ACTION_HELP  0x08

extern int do_mtime;

struct metaentry {
	struct metaentry *next;
	char *path;
	char *owner;
	char *group;
        mode_t mode;
	time_t mtime;
        long mtimensec;
	unsigned int xattrs;
	char **xattr_names;
	ssize_t *xattr_lvalues;
	char **xattr_values;
};

