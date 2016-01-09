#!/bin/sh

METASTORE_PRETAG_VER=v1.0.999

cd "${0%/*}" && \
METASTORE_VER="$(\
{ git describe --tags 2>/dev/null \
  || grep -no '^v[0-9][^ ]*' NEWS \
  || echo $METASTORE_PRETAG_VER; \
} | sed '/^1:/s,,,;/:/{s,[^:]*:,,;s,$,+,;};q' \
)"

echo "$METASTORE_VER_PREFIX$METASTORE_VER$METASTORE_VER_SUFFIX"
