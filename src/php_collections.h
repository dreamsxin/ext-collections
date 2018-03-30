//
// ext-collections/php_collections.h
//
// @Author CismonX
//

#ifndef PHP_COLLECTIONS_H
#define PHP_COLLECTIONS_H

extern zend_module_entry collections_module_entry;
#define phpext_collections_ptr &collections_module_entry

#define PHP_COLLECTIONS_VERSION "0.1.0"

#ifdef PHP_WIN32
#define PHP_COLLECTIONS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PHP_COLLECTIONS_API __attribute__ ((visibility("default")))
#else
#define PHP_COLLECTIONS_API
#endif

extern zend_string* collection_property_name;
extern zend_string* pair_first_name;
extern zend_string* pair_second_name;

extern PHP_COLLECTIONS_API zend_class_entry* collections_collection_ce;
extern PHP_COLLECTIONS_API zend_class_entry* collections_pair_ce;

extern zend_object_handlers* collection_handlers;

int count_collection(zval* obj, zend_long* count);

extern const zend_function_entry collection_methods[];
extern const zend_function_entry pair_methods[];

#ifdef ZTS
#include "TSRM.h"
#endif

#if defined(ZTS) && defined(COMPILE_DL_COLLECTIONS)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif // !PHP_COLLECTIONS_FE_H