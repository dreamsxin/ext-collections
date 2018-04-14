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

#if PHP_VERSION_ID < 70300
#define GC_ADDREF(p) ++GC_REFCOUNT(p)
#define GC_DELREF(p) --GC_REFCOUNT(p)
#endif

extern PHP_COLLECTIONS_API zend_class_entry* collections_collection_ce;
extern PHP_COLLECTIONS_API zend_class_entry* collections_pair_ce;

extern zend_object_handlers* collection_handlers;

int count_collection(zval* obj, zend_long* count);
int collection_offset_exists(zval* object, zval* offset, int check_empty);
void collection_offset_set(zval* object, zval* offset, zval* value);
zval* collection_offset_get(zval* object, zval* offset, int type, zval* rv);
void collection_offset_unset(zval* object, zval* offset);

extern const zend_function_entry collection_methods[];
extern const zend_function_entry pair_methods[];

#ifdef ZTS
#include "TSRM.h"
#endif

#if defined(ZTS) && defined(COMPILE_DL_COLLECTIONS)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif // !PHP_COLLECTIONS_H