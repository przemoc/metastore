void mentries_recurse_path(const char *, struct metaentry **);
void mentries_tofile(const struct metaentry *, const char *);
void mentries_fromfile(struct metaentry **, const char *);
int mentry_find_xattr(struct metaentry *, struct metaentry *, int);

#define DIFF_NONE  0x00
#define DIFF_OWNER 0x01
#define DIFF_GROUP 0x02
#define DIFF_MODE  0x04
#define DIFF_TYPE  0x08
#define DIFF_MTIME 0x10
#define DIFF_XATTR 0x20
#define DIFF_ADDED 0x40
#define DIFF_DELE  0x80
void mentries_compare(struct metaentry *, struct metaentry *,
        void (*printfunc)(struct metaentry *, struct metaentry *, int));


