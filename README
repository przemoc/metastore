metastore
=========

metastore is a tool to store the metadata of files/directories/links in
a file tree to a separate file and to later compare and apply the stored
metadata to said file tree.

It was originally written as a supplement to git, which does not store
all metadata, making it unsuitable for e.g. storing /etc in a
repository.

metastore can also be helpful if you want to create a tarball of a file
tree and make sure that "everything" (e.g. xattrs, mtime, owner, group)
is stored along with the files.


Stored metadata
---------------

metastore stores following metadata in its files:

- owner,
- group,
- permissions,
- xattrs,
- mtime - optionally.


Usage
-----

See metastore.txt file, which is plain-text version of the manual page.

If you want to use metastore within git repository, then consider
copying exemplary scripts from examples/hooks/ directory to hooks
subdirectory in your git directory, and make them executable.
You can also tune them if it's necessary. But.
Before using, please read the warning in the comments of pre-commit hook.
Mind that merge conflicts can only be solved manually, because metastore
file is binary in its current form (there's a plan to fix it in future).
Dump action can be really helpful in such cases.


File format
-----------

See FILEFORMAT file, which describes internals of metastore file.


Requirements
------------

- Linux
- GNU make
- C99 compiler, like gcc or clang
- libbsd


Download
--------

You can obtain any released version (tarball and signature) from:

    ftp://ftp.przemoc.net/pub/software/utils/metastore/
    http://ftp.przemoc.net/pub/software/utils/metastore/

Signer should be the current maintainer, see AUTHORS file.

Alternatively, you can get it from:

    https://github.com/przemoc/metastore/releases/

If you want to see the latest source code, then git clone following URL:

    https://github.com/przemoc/metastore


Building
--------

Simply run `make` from project's root directory.

Building out-of-tree is supported out-of-the-box. Go to your chosen
build directory and run there:

    $ make -f path/to/metastore/Makefile


Installation
------------

Run `make install`.  Default settings for installation are:

     PREFIX      = /usr/local

     EXECPREFIX  = ${PREFIX}
                   (/usr/local)
     BINDIR      = ${EXECPREFIX}/bin
                   (/usr/local/bin)
     DATAROOTDIR = ${PREFIX}/share
                   (/usr/local/share)
     DOCDIR      = ${DATAROOTDIR}/doc/metastore
                   (/usr/local/share/doc/metastore)
     MANDIR      = ${DATAROOTDIR}/man
                   (/usr/local/share/man)

You can always change them, e.g.:

    $ make install PREFIX=/usr

DESTDIR is also supported.


Reporting issues
----------------

Please use the issue tracker provided by GitHub to send bug reports
or feature requests.

https://github.com/przemoc/metastore/issues

If you're sending a bug report, then please provide some basic info:

- What system do you have?  
  (`uname -a`, `lsb_release -drc`)
- What gcc version have you used for building metastore?  
  (`gcc -v`)
- Do you use any custom FLAGS during build?  
  (`set | grep FLAGS=`)
- What libc implementation and version are you using?
- What metastore version are you using?  
  (`metastore -V`)
- What filesystem do you use and how it is mounted?  
  (`mount | grep $(df . | awk 'NR==2{print$1}')`)


Mailing list
------------

metastore has one official read-only mailing list dedicated to
announcements like new version releases or other important updates.

You can subscribe it using following web page:
https://www.freelists.org/list/metastore-announce

or by sending email to metastore-announce-request@freelists.org
with `subscribe` in the Subject field.

Archive for metastore-announce mailing list is available at:
https://www.freelists.org/archive/metastore-announce


License
-------

The project is licensed under the terms of the GNU GPL v2 only license.
See LICENSE.GPLv2 file for the full license text.
