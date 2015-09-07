#
# Copyright (C) 2007 David Härdeman <david@hardeman.nu>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; only version 2 of the License is applicable.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#
# Generic settings
#
CC              = gcc
CFLAGS         += -g -Wall -pedantic -std=c99 -D_FILE_OFFSET_BITS=64 -O2
LDFLAGS        += -lbsd
INCLUDES        =
INSTALL         = install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA    = ${INSTALL} -m 644
COMPILE         = $(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS)
LINK            = $(CC) $(CFLAGS) $(LDFLAGS)
OBJECTS         = utils.o metastore.o metaentry.o
HEADERS         = utils.h metastore.h metaentry.h

DESTDIR        ?=
prefix         	= /usr
usrbindir       = ${prefix}/bin
mandir          = ${prefix}/share/man

#
# Targets
#

all: metastore
.DEFAULT: all


%.o: %.c $(HEADERS)
	$(COMPILE) -o $@ -c $<


metastore: $(OBJECTS)
	$(LINK) -o $@ $^


install: all
	$(INSTALL_DATA) -D metastore.1 $(DESTDIR)$(mandir)/man1/metastore.1
	$(INSTALL_PROGRAM) -D metastore $(DESTDIR)$(usrbindir)/metastore


uninstall:
	- rm -f $(DESTDIR)$(mandir)/man1/metastore.1
	- rm -f $(DESTDIR)$(usrbindir)/metastore


clean:
	- rm -f *.o metastore


.PHONY: install uninstall clean all

