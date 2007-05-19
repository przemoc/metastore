#
# Copyright (C) 2007 David Härdeman <david@hardeman.nu>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#
# Generic settings
#
CC       = gcc
CFLAGS   = -g -Wall
LDFLAGS  =
INCLUDES =
COMPILE  = $(CC) $(INCLUDES) $(CFLAGS)
LINK     = $(CC) $(CFLAGS) $(LDFLAGS)
OBJECTS = utils.o metastore.o metaentry.o
HEADERS = utils.h metastore.h metaentry.h

#
# Targets
#

%.o: %.c $(HEADERS)
	$(COMPILE) -o $@ -c $<

metastore: $(OBJECTS)
	$(LINK) -o $@ $^

all: metastore
.PHONY: all
.DEFAULT: all

clean:
	rm -f *.o metastore
.PHONY: clean

