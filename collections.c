//
// ext-collections/collections.c
//
// @Author CismonX
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>

#include "php_collections.h"

PHP_MINIT_FUNCTION(collections)
{
    INIT_CLASS_ENTRY_EX(collections_collection_ce, "Collection", strlen("Collection"), collection_methods);
    zend_register_internal_class(&collections_collection_ce);
    INIT_CLASS_ENTRY_EX(collections_pair_ce, "Pair", strlen("Pair"), pair_methods);
    zend_register_internal_class(&collections_pair_ce);
    return SUCCESS;
}

PHP_RINIT_FUNCTION(collections)
{
#if defined(COMPILE_DL_COLLECTIONS) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
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
    PHP_RINIT(collections),
    NULL,
    PHP_MINFO(collections),
    PHP_COLLECTIONS_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_COLLECTIONS
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(collections)
#endif
