//
// ext-collections/collections.c
//
// @Author CismonX
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_collections.h"

#include <zend_interfaces.h>
#include <ext/standard/info.h>

#define COLLECTIONS_CE_INIT(cls, name)                                \
    zend_class_entry cls##_ce;                                        \
    INIT_CLASS_ENTRY(cls##_ce, name, cls##_methods);                  \
    collections_##cls##_ce = zend_register_internal_class(&cls##_ce)
#define COLLECTIONS_CONST_DECLARE(cls, name, val)                     \
    zend_declare_class_constant_long(collections_##cls##_ce, name, sizeof(name) - 1, val)
#define COLLECTIONS_PROP_DECLARE(cls, name, flags)                    \
    zend_declare_property_null(collections_##cls##_ce, name, sizeof(name) - 1, flags)

zend_object_handlers collection_handlers;

zend_class_entry* collections_collection_ce;
zend_class_entry* collections_pair_ce;

ZEND_DECLARE_MODULE_GLOBALS(collections)

static zend_always_inline void collection_ce_init()
{
    COLLECTIONS_CE_INIT(collection, "Collection");
    COLLECTIONS_CONST_DECLARE(collection, "COMPARE_NATRUAL", PHP_COLLECTIONS_COMPARE_NATURAL);
    COLLECTIONS_CONST_DECLARE(collection, "FOLD_CASE", PHP_COLLECTIONS_FOLD_CASE);
    zend_class_implements(collections_collection_ce,
#if PHP_VERSION_ID < 70200
        1,
#else
        2, zend_ce_countable,
#endif
        zend_ce_arrayaccess);
    memcpy(&collection_handlers, &std_object_handlers, sizeof(zend_object_handlers));
    collection_handlers.unset_dimension = collection_offset_unset;
    collection_handlers.unset_property = collection_property_unset;
    collection_handlers.write_dimension = collection_offset_set;
    collection_handlers.write_property = collection_property_set;
    collection_handlers.read_dimension = collection_offset_get;
    collection_handlers.read_property = collection_property_get;
    collection_handlers.has_dimension = collection_offset_exists;
    collection_handlers.has_property = collection_property_exists;
    collection_handlers.count_elements = count_collection;
}

static zend_always_inline void pair_ce_init()
{
    COLLECTIONS_CE_INIT(pair, "Pair");
    COLLECTIONS_PROP_DECLARE(pair, "first", ZEND_ACC_PUBLIC);
    COLLECTIONS_PROP_DECLARE(pair, "second", ZEND_ACC_PUBLIC);
}

PHP_MINIT_FUNCTION(collections)
{
#ifdef ZTS
    ZEND_INIT_MODULE_GLOBALS(collections, NULL, NULL);
#endif
    collection_ce_init();
    pair_ce_init();
    return SUCCESS;
}

PHP_MINFO_FUNCTION(collections)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "collections support", "enabled");
    php_info_print_table_end();
}

zend_module_entry collections_module_entry = {
    STANDARD_MODULE_HEADER,
    "collections",
    NULL,
    PHP_MINIT(collections),
    NULL,
    NULL,
    NULL,
    PHP_MINFO(collections),
    PHP_COLLECTIONS_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_COLLECTIONS
ZEND_GET_MODULE(collections)
#endif
