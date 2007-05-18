CC       = gcc
CFLAGS   = -g -Wall
LDFLAGS  =
INCLUDES =
COMPILE  = $(CC) $(INCLUDES) $(CFLAGS)
LINK     = $(CC) $(CFLAGS) $(LDFLAGS)

objects = utils.o metastore.o metaentry.o

%.o: %.c
	$(COMPILE) -o $@ -c $<

metastore: $(objects)
	$(LINK) -o $@ $^

clean:
	rm -f *.o metastore
.PHONY: clean
