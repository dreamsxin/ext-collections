//
// ext-collections/collections.c
//
// @Author CismonX
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include <zend_interfaces.h>
#include <ext/standard/info.h>

#include "php_collections.h"

zend_object_handlers collection_handlers;

zend_class_entry* collections_collection_ce;
zend_class_entry* collections_pair_ce;

ZEND_DECLARE_MODULE_GLOBALS(collections)

static zend_always_inline void collection_ce_init()
{
    zend_class_entry collection_ce;
    INIT_CLASS_ENTRY_EX(collection_ce, "Collection", sizeof "Collection" - 1, collection_methods);
    collections_collection_ce = zend_register_internal_class(&collection_ce);
    zend_declare_class_constant_long(collections_collection_ce,
        "COMPARE_NATRUAL", sizeof "COMPARE_NATRUAL" - 1, PHP_COLLECTIONS_COMPARE_NATURAL);
    zend_declare_class_constant_long(collections_collection_ce,
        "FOLD_CASE", sizeof "FOLD_CASE" - 1, PHP_COLLECTIONS_FOLD_CASE);
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
    zend_class_entry pair_ce;
    INIT_CLASS_ENTRY_EX(pair_ce, "Pair", sizeof "Pair" - 1, pair_methods);
    collections_pair_ce = zend_register_internal_class(&pair_ce);
    zend_declare_property_null(collections_pair_ce, "first", sizeof "first" - 1, ZEND_ACC_PUBLIC);
    zend_declare_property_null(collections_pair_ce, "second", sizeof "second" - 1, ZEND_ACC_PUBLIC);
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
