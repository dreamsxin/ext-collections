PHP_ARG_ENABLE(collections, for collections support,
[  --enable-collections           Enable collections support])

PHP_ARG_ENABLE(collections, for debug support,
[  --enable-collections-debug     Compile with debug symbols])

if test "$PHP_COLLECTIONS" != "no"; then
  if test "$PHP_COLLECTIONS_DEBUG" != "no"; then
    EXTRA_CFLAGS="-O2"
  else
    EXTRA_CFLAGS="-g -O0"
  fi
  COLLECTIONS_SRC="collections.c collections_fe.c collections_methods.c"
  PHP_NEW_EXTENSION(collections, $COLLECTIONS_SRC, $ext_shared,, $EXTRA_CFLAGS)
fi
