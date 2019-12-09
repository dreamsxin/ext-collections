//
// ext-collections/php_collections.h
//
// @Author CismonX
//

#ifndef PHP_COLLECTIONS_H
#define PHP_COLLECTIONS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>

extern zend_module_entry collections_module_entry;
#define phpext_collections_ptr          &collections_module_entry

#define PHP_COLLECTIONS_VERSION         "0.1.0"

#ifdef PHP_WIN32
#define PHP_COLLECTIONS_API             __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PHP_COLLECTIONS_API             __attribute__ ((visibility("default")))
#else
#define PHP_COLLECTIONS_API
#endif

#if PHP_VERSION_ID < 70100
#error "This extension requires PHP 7.1 and above."
#endif

#if PHP_VERSION_ID >= 70400
typedef zval* zobj_write_prop_ret_t;
#else
typedef void zobj_write_prop_ret_t;
#endif

#define PHP_COLLECTIONS_COMPARE_NATURAL (1 << 0)
#define PHP_COLLECTIONS_FOLD_CASE       (1 << 1)

ZEND_BEGIN_MODULE_GLOBALS(collections)
    zend_fcall_info* fci;
    zend_fcall_info_cache* fcc;
    zval* ref;
    compare_func_t cmp;
ZEND_END_MODULE_GLOBALS(collections)

ZEND_EXTERN_MODULE_GLOBALS(collections)

#ifdef ZTS
#ifdef COMPILE_DL_COLLECTIONS
ZEND_TSRMLS_CACHE_EXTERN()
#endif
#define COLLECTIONS_G(v)                TSRMG(collections_globals_id, zend_collections_globals*, v)
#else
#define COLLECTIONS_G(v)                (collections_globals.v)
#endif

extern PHP_COLLECTIONS_API zend_class_entry* collections_collection_ce;
extern PHP_COLLECTIONS_API zend_class_entry* collections_pair_ce;

extern zend_object_handlers collection_handlers;

int collection_count_elements(zval* obj, zend_long* count);
int collection_has_dimension(zval* object, zval* offset, int check_empty);
void collection_write_dimension(zval* object, zval* offset, zval* value);
zval* collection_read_dimension(zval* object, zval* offset, int type, zval* rv);
void collection_unset_dimension(zval* object, zval* offset);
int collection_has_property(zval* object, zval* member, int has_set_exists, void**);
zobj_write_prop_ret_t collection_write_property(zval* object, zval* member, zval* value, void**);
zval* collection_read_property(zval* object, zval* member, int type, void**, zval* rv);
void collection_unset_property(zval* object, zval* member, void**);

extern const zend_function_entry collection_methods[];
extern const zend_function_entry pair_methods[];

#endif // !PHP_COLLECTIONS_H
