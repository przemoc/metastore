#define SIGNATURE    "MeTaSt00r3"
#define SIGNATURELEN 10
#define VERSION      "\0\0\0\0\0\0\0\0"
#define VERSIONLEN   8
#define METAFILE     "./.metadata"

#define MSG_NORMAL    0
#define MSG_DEBUG     1
#define MSG_QUIET    -1
#define MSG_CRITICAL -2

#define ACTION_DIFF  0x01
#define ACTION_SAVE  0x02
#define ACTION_APPLY 0x04
#define ACTION_HELP  0x08

#define DIFF_NONE  0x00
#define DIFF_OWNER 0x01
#define DIFF_GROUP 0x02
#define DIFF_MODE  0x04
#define DIFF_TYPE  0x08
#define DIFF_MTIME 0x10
#define DIFF_XATTR 0x20
#define DIFF_ADDED 0x40
#define DIFF_DELE  0x80

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

