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
LDFLAGS        +=
LIBS           += -lbsd
INCLUDES        =
INSTALL         = install
INSTALL_PROGRAM = ${INSTALL} -c
INSTALL_DATA    = ${INSTALL} -c -m 644
COMPILE         = $(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS)
LINK            = $(CC) $(CFLAGS) $(LDFLAGS)
OBJECTS         = utils.o metastore.o metaentry.o
HEADERS         = utils.h metastore.h metaentry.h
MANPAGES        = man1/metastore.1

PROJ_DIR       := $(dir $(lastword $(MAKEFILE_LIST)))
SRCS_DIR       := $(PROJ_DIR)src/
MANS_DIR       := $(PROJ_DIR)

DESTDIR        ?=
prefix         	= /usr
usrbindir       = ${prefix}/bin
mandir          = ${prefix}/share/man

vpath %.c $(SRCS_DIR)
vpath %.h $(SRCS_DIR)
vpath %.1 $(MANS_DIR)

#
# Targets
#

all: metastore
.DEFAULT: all


%.o: %.c $(HEADERS)
	$(COMPILE) -o $@ -c $<


metastore: $(OBJECTS)
	$(LINK) -o $@ $^ $(LIBS)


install: all $(MANPAGES)
	$(INSTALL) -d $(DESTDIR)$(mandir)/man1/
	$(INSTALL_DATA) $(filter %.1,$^) $(DESTDIR)$(mandir)/man1/
	$(INSTALL) -d $(DESTDIR)$(usrbindir)/
	$(INSTALL_PROGRAM) metastore $(DESTDIR)$(usrbindir)/


uninstall:
	- rm -f $(addprefix $(DESTDIR)$(mandir)/,$(MANPAGES))
	- rm -f $(DESTDIR)$(usrbindir)/metastore


clean:
	- rm -f *.o metastore


.PHONY: install uninstall clean all

