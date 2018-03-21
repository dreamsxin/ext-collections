//
// ext-collections/php_collections.h
//
// @Author CismonX
//

#ifndef PHP_COLLECTIONS_H
#define PHP_COLLECTIONS_H

extern zend_module_entry collections_module_entry;
#define phpext_collections_ptr &collections_module_entry

extern zend_class_entry collections_collection_ce;
extern zend_class_entry collections_pair_ce;

const zend_function_entry collection_methods[];
const zend_function_entry pair_methods[];

#define PHP_COLLECTIONS_VERSION "0.1.0"

#ifdef PHP_WIN32
#define PHP_COLLECTIONS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PHP_COLLECTIONS_API __attribute__ ((visibility("default")))
#else
#define PHP_COLLECTIONS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#if defined(ZTS) && defined(COMPILE_DL_COLLECTIONS)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif // !PHP_COLLECTIONS_FE_H