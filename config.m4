PHP_ARG_ENABLE(collections, for collections support,
[  --enable-collections           Enable collections support])

if test "$PHP_COLLECTIONS" != "no"; then
  COLLECTIONS_SRC="src/collections.c
    src/collections_me.c
    src/collections_methods.c"
  PHP_NEW_EXTENSION(collections, $COLLECTIONS_SRC, $ext_shared)
fi
