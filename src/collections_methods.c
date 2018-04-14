//
// ext-collections/collections_methods.c
//
// @Author CismonX
//

#include <php.h>

#include "php_collections.h"
#include "php_collections_me.h"

#define NEW_OBJ(name, ce, object_handlers) \
    zend_object* (name) = (zend_object*)ecalloc(1, sizeof(zend_object) + \
        zend_object_properties_size(ce)); \
    zend_object_std_init(name, ce); \
    object_properties_init(name, ce); \
    (name)->handlers = object_handlers;
#define NEW_COLLECTION_OBJ(name) \
    NEW_OBJ(name, collections_collection_ce, collection_handlers)
#define NEW_PAIR_OBJ(name) \
    NEW_OBJ(name, collections_pair_ce, &std_object_handlers)

#define IS_COLLECTION_P(zval) \
    Z_TYPE_P(zval) == IS_OBJECT && Z_OBJCE_P(zval) == collections_collection_ce
#define IS_PAIR(zval) \
    EXPECTED(Z_TYPE(zval) == IS_OBJECT) && EXPECTED(Z_OBJCE(zval) == collections_pair_ce)

#define OBJ_PROPERTY_UPDATE(ce, obj, property_name, value) \
    zend_update_property(ce, obj, #property_name, sizeof #property_name - 1, value)
#define OBJ_PROPERTY_FETCH(ce, obj, property_name) \
    zend_read_property(ce, obj, #property_name, sizeof #property_name - 1, 1, &rv)
#define COLLECTION_UPDATE(obj, value) OBJ_PROPERTY_UPDATE(collections_collection_ce, obj, _, value)
#define COLLECTION_UPDATE_EX(value) COLLECTION_UPDATE(getThis(), value)
#define COLLECTION_FETCH(obj) OBJ_PROPERTY_FETCH(collections_collection_ce, obj, _)
#define COLLECTION_FETCH_EX() COLLECTION_FETCH(getThis())
#define PAIR_UPDATE_FIRST(obj, value) OBJ_PROPERTY_UPDATE(collections_pair_ce, obj, first, value)
#define PAIR_UPDATE_SECOND(obj, value) OBJ_PROPERTY_UPDATE(collections_pair_ce, obj, second, value)
#define PAIR_FETCH_FIRST(obj) OBJ_PROPERTY_FETCH(collections_pair_ce, obj, first)
#define PAIR_FETCH_SECOND(obj)  OBJ_PROPERTY_FETCH(collections_pair_ce, obj, second)

#define INIT_FCI() \
    zval params[2], rv, retval; \
    fci.param_count = 2; \
    fci.retval = &retval; \
    fci.params = params;

#define CALLBACK_KEYVAL_INVOKE(params, bucket) \
    ZVAL_COPY_VALUE(&params[0], &bucket->val); \
    ZVAL_COPY(&params[0], &(bucket)->val); \
    if ((bucket)->key) \
        ZVAL_STR(&params[1], (bucket)->key); \
    else \
        ZVAL_LONG(&params[1], (bucket)->h); \
    zend_call_function(&fci, &fcc); \
    zval_ptr_dtor(&params[0])

#define INIT_EQUAL_CHECK_FUNC(val) \
    int (*equal_check_func)(zval*, zval*); \
    if (Z_TYPE_P(val) == IS_LONG) \
        equal_check_func = fast_equal_check_long; \
    else if (Z_TYPE_P(val) == IS_STRING) \
        equal_check_func = fast_equal_check_string; \
    else \
        equal_check_func = fast_equal_check_function;

#define PHP_COLLECTIONS_ERROR(type, msg) php_error_docref(NULL, type, msg)
#define ERR_BAD_ARGUMENT_TYPE() PHP_COLLECTIONS_ERROR(E_WARNING, "Bad argument type")
#define ERR_BAD_KEY_TYPE() PHP_COLLECTIONS_ERROR(E_WARNING, "Key must be integer or string")
#define ERR_BAD_CALLBACK_RETVAL() PHP_COLLECTIONS_ERROR(E_WARNING, "Bad callback return value")
#define ERR_BAD_SIZE() PHP_COLLECTIONS_ERROR(E_WARNING, "Size must be non-negative")
#define ERR_BAD_INDEX() PHP_COLLECTIONS_ERROR(E_WARNING, "Index must be non-negative")
#define ERR_NOT_ARITHMETIC() PHP_COLLECTIONS_ERROR(E_WARNING, "Elements should be int or double")

#define ELEMENTS_VALIDATE(elements) \
    if (IS_COLLECTION_P(elements)) { \
        zval rv; \
        (elements) = COLLECTION_FETCH(elements); \
    } else if (UNEXPECTED(Z_TYPE_P(elements) != IS_ARRAY)) { \
        ERR_BAD_ARGUMENT_TYPE(); \
        RETVAL_NULL(); \
    }

#define ARRAY_NEW(name, size) \
    zend_array* (name) = (zend_array*)emalloc(sizeof(zend_array)); \
    zend_hash_init(name, size, NULL, ZVAL_PTR_DTOR, 0)
#define ARRAY_NEW_EX(name, other) \
    ARRAY_NEW(name, zend_hash_num_elements(Z_ARRVAL_P(other)))
#define ARRAY_CLONE(dest, src) \
    ARRAY_NEW_EX(dest, src); \
    zend_hash_copy(dest, Z_ARRVAL_P(src), NULL)

// Compatible with PHP 7.2
#if PHP_VERSION_ID >= 70200
#define ALLOW_COW_VIOLATION(ht) HT_ALLOW_COW_VIOLATION(ht)
#else
#define ALLOW_COW_VIOLATION(ht)
#endif

#define RETVAL_NEW_COLLECTION(collection) \
    do { \
        ALLOW_COW_VIOLATION(collection); \
        NEW_COLLECTION_OBJ(obj); \
        zval _retval; \
        ZVAL_OBJ(&_retval, obj); \
        zval _property; \
        ZVAL_ARR(&_property, collection); \
        COLLECTION_UPDATE(&_retval, &_property); \
        zval_ptr_dtor(&_property); \
        RETVAL_OBJ(obj); \
    } while (0)

#define RETURN_NEW_COLLECTION(collection) { \
        RETVAL_NEW_COLLECTION(collection); \
        return; \
    }

int count_collection(zval* obj, zend_long* count)
{
    zval rv;
    zval* current = COLLECTION_FETCH(obj);
    *count = zend_hash_num_elements(Z_ARRVAL_P(current));
    return SUCCESS;
}

int collection_offset_exists(zval* object, zval* offset, int check_empty)
{
    zval rv;
    zval* current = COLLECTION_FETCH(object);
    if (check_empty)
        return zend_hash_num_elements(Z_ARRVAL_P(current)) == 0;
    if (Z_TYPE_P(offset) == IS_LONG)
        return zend_hash_index_exists(Z_ARRVAL_P(current), Z_LVAL_P(offset));
    if (Z_TYPE_P(offset) == IS_STRING)
        return zend_hash_exists(Z_ARRVAL_P(current), Z_STR_P(offset));
    return 0;
}

void collection_offset_set(zval* object, zval* offset, zval* value)
{
    zval rv;
    zval* current = COLLECTION_FETCH(object);
    if (Z_TYPE_P(offset) == IS_LONG)
        zend_hash_index_update(Z_ARRVAL_P(current), Z_LVAL_P(offset), value);
    else if (Z_TYPE_P(offset) == IS_STRING)
        zend_hash_update(Z_ARRVAL_P(current), Z_STR_P(offset), value);
}

zval* collection_offset_get(zval* object, zval* offset, int type, zval* retval)
{
    // Note that we don't handle type. So don't do any fancy things with Collection
    // such as fetching a reference of a value, etc.
    zval rv;
    zval* current = COLLECTION_FETCH(object);
    zval* found = NULL;
    if (Z_TYPE_P(offset) == IS_LONG)
        found = zend_hash_index_find(Z_ARRVAL_P(current), Z_LVAL_P(offset));
    else if (Z_TYPE_P(offset) == IS_STRING)
        found = zend_hash_find(Z_ARRVAL_P(current), Z_STR_P(offset));
    ZVAL_COPY(retval, found);
    return retval;
}

void collection_offset_unset(zval* object, zval* offset)
{
    zval rv;
    zval* current = COLLECTION_FETCH(object);
    if (Z_TYPE_P(offset) == IS_LONG)
        zend_hash_index_del(Z_ARRVAL_P(current), Z_LVAL_P(offset));
    else if (Z_TYPE_P(offset) == IS_STRING)
        zend_hash_del(Z_ARRVAL_P(current), Z_STR_P(offset));
}

PHP_METHOD(Collection, __construct) {}

PHP_METHOD(Collection, addAll)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements);
    zval rv;
    zval* current = COLLECTION_FETCH_EX();
    ZEND_HASH_FILL_PACKED(Z_ARRVAL_P(current))
        ZEND_HASH_FOREACH_VAL_IND(Z_ARRVAL_P(elements), zval* val)
            ZEND_HASH_FILL_ADD(val);
        ZEND_HASH_FOREACH_END();
    ZEND_HASH_FILL_END();
}

PHP_METHOD(Collection, all)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval))
            RETURN_FALSE;
    ZEND_HASH_FOREACH_END();
    RETURN_TRUE;
}

PHP_METHOD(Collection, any)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
            RETURN_TRUE;
    ZEND_HASH_FOREACH_END();
    RETURN_FALSE;
}

PHP_METHOD(Collection, associate)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (IS_PAIR(retval)) {
            zval* key = PAIR_FETCH_FIRST(&retval);
            zval* value = PAIR_FETCH_SECOND(&retval);
            if (Z_TYPE_P(key) == IS_LONG)
                zend_hash_index_add(new_collection, Z_LVAL_P(key), value);
            else if (Z_TYPE_P(key) == IS_STRING)
                zend_hash_add(new_collection, Z_STR_P(key), value);
            else if (Z_TYPE_P(key) == IS_NULL)
                zend_hash_next_index_insert(new_collection, value);
            else
                ERR_BAD_KEY_TYPE();
            zval_ptr_dtor(&retval);
        } else
            ERR_BAD_CALLBACK_RETVAL();
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, associateTo)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    zval* dest;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_OBJECT_OF_CLASS(dest, collections_collection_ce)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    zend_array* dest_arr = Z_ARRVAL_P(COLLECTION_FETCH(dest));
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (IS_PAIR(retval)) {
            zval* key = PAIR_FETCH_FIRST(&retval);
            zval* value = PAIR_FETCH_SECOND(&retval);
            if (Z_TYPE_P(key) == IS_LONG)
                zend_hash_index_add(dest_arr, Z_LVAL_P(key), value);
            else if (Z_TYPE_P(key) == IS_STRING)
                zend_hash_add(dest_arr, Z_STR_P(key), value);
            else if (Z_TYPE_P(key) == IS_NULL)
                zend_hash_next_index_insert(dest_arr, value);
            else
                ERR_BAD_KEY_TYPE();
            zval_ptr_dtor(&retval);
        } else
            ERR_BAD_CALLBACK_RETVAL();
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, associateBy)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (Z_TYPE(retval) == IS_LONG)
            zend_hash_index_add(new_collection, Z_LVAL(retval), &bucket->val);
        else if (Z_TYPE(retval) == IS_STRING)
            zend_hash_add(new_collection, Z_STR(retval), &bucket->val);
        else
            ERR_BAD_CALLBACK_RETVAL();
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, associateByTo)
{
    zval* dest;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_OBJECT_OF_CLASS(dest, collections_collection_ce)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    zend_array* dest_arr = Z_ARRVAL_P(COLLECTION_FETCH(dest));
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (Z_TYPE(retval) == IS_LONG)
            zend_hash_index_add(dest_arr, Z_LVAL(retval), &bucket->val);
        else if (Z_TYPE(retval) == IS_STRING)
            zend_hash_add(dest_arr, Z_STR(retval), &bucket->val);
        else
            ERR_BAD_CALLBACK_RETVAL();
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, average)
{
    zval rv;
    zval* current = COLLECTION_FETCH_EX();
    double sum = 0;
    ZEND_HASH_FOREACH_VAL_IND(Z_ARRVAL_P(current), zval* val)
        if (Z_TYPE_P(val) == IS_LONG)
            sum += Z_LVAL_P(val);
        else if (Z_TYPE_P(val) == IS_DOUBLE)
            sum += Z_DVAL_P(val);
        else {
            ERR_NOT_ARITHMETIC();
            RETURN_NULL();
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_DOUBLE(sum / zend_hash_num_elements(Z_ARRVAL_P(current)));
}

PHP_METHOD(Collection, containsAll)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements);
    zval rv;
    zval* current = COLLECTION_FETCH_EX();
    ZEND_HASH_FOREACH_VAL_IND(Z_ARRVAL_P(elements), zval* element)
        INIT_EQUAL_CHECK_FUNC(element);
        int result = 0;
        ZEND_HASH_FOREACH_VAL_IND(Z_ARRVAL_P(current), zval* val)
            if (result = equal_check_func(element, val))
                break;
        ZEND_HASH_FOREACH_END();
        if (result == 0)
            RETURN_FALSE;
    ZEND_HASH_FOREACH_END();
    RETURN_TRUE;
}

PHP_METHOD(Collection, containsKey)
{
    zval* key;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(key)
    ZEND_PARSE_PARAMETERS_END();
    zval rv;
    zval* current = COLLECTION_FETCH_EX();
    if (Z_TYPE_P(key) == IS_LONG)
        RETURN_BOOL(zend_hash_index_exists(Z_ARRVAL_P(current), Z_LVAL_P(key)));
    if (Z_TYPE_P(key) == IS_STRING)
        RETURN_BOOL(zend_hash_exists(Z_ARRVAL_P(current), Z_STR_P(key)));
    ERR_BAD_KEY_TYPE();
    RETVAL_FALSE;
}

PHP_METHOD(Collection, containsValue)
{
    zval* element;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(element)
    ZEND_PARSE_PARAMETERS_END();
    zval rv;
    zval* current = COLLECTION_FETCH_EX();
    INIT_EQUAL_CHECK_FUNC(element);
    ZEND_HASH_FOREACH_VAL_IND(Z_ARRVAL_P(current), zval* val)
        if (equal_check_func(element, val))
            RETURN_TRUE;
    ZEND_HASH_FOREACH_END();
    RETURN_FALSE;
}

PHP_METHOD(Collection, copyOf)
{
    zend_long new_size = -1;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(new_size);
    ZEND_PARSE_PARAMETERS_END();
    zval rv;
    zval* current = COLLECTION_FETCH_EX();
    if (EX_NUM_ARGS() == 0) {
        ARRAY_CLONE(new_collection, current);
        RETURN_NEW_COLLECTION(new_collection);
    }
    if (UNEXPECTED(new_size < 0)) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    if (new_size == 0) {
        ARRAY_NEW(new_collection, 0);
        RETURN_NEW_COLLECTION(new_collection);
    }
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        if (bucket->key)
            zend_hash_add(new_collection, bucket->key, &bucket->val);
        else
            zend_hash_index_add(new_collection, bucket->h, &bucket->val);
        if (--new_size == 0)
            break;
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, copyOfRange)
{
    zend_long from_idx, num_elements;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(from_idx)
        Z_PARAM_LONG(num_elements)
    ZEND_PARSE_PARAMETERS_END();
    if (from_idx < 0) {
        ERR_BAD_INDEX();
        RETURN_NULL();
    }
    if (num_elements < 0) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    zval rv;
    zval* current = COLLECTION_FETCH_EX();
    ARRAY_NEW(new_collection, num_elements);
    Bucket* bucket = Z_ARRVAL_P(current)->arData;
    Bucket* end = bucket + Z_ARRVAL_P(current)->nNumUsed;
    for (bucket += from_idx; num_elements > 0 && bucket < end; ++bucket, --num_elements)
        if (bucket->key)
            zend_hash_add(new_collection, bucket->key, &bucket->val);
        else
            zend_hash_next_index_insert(new_collection, &bucket->val);
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, count)
{
    zend_long count;
    count_collection(getThis(), &count);
    RETVAL_LONG(count);
}

PHP_METHOD(Collection, distinct)
{
    
}

PHP_METHOD(Collection, distinctBy)
{
    
}

PHP_METHOD(Collection, drop)
{
    zend_long n;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(n)
    ZEND_PARSE_PARAMETERS_END();
    if (n < 0) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    zval rv;
    zval* current = COLLECTION_FETCH_EX();
    ARRAY_CLONE(new_collection, current);
    Bucket* bucket = new_collection->arData;
    Bucket* end = bucket + new_collection->nNumUsed;
    for (; n > 0 && bucket < end; ++bucket, --n) {
        if (Z_REFCOUNTED(bucket->val))
            GC_ADDREF(Z_COUNTED(bucket->val));
        zend_hash_del_bucket(new_collection, bucket);
    }
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, dropLast)
{
    zend_long n;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(n)
    ZEND_PARSE_PARAMETERS_END();
    if (n < 0) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    zval rv;
    zval* current = COLLECTION_FETCH_EX();
    ARRAY_CLONE(new_collection, current);
    unsigned idx = new_collection->nNumUsed;
    for (; n > 0 && idx > 0; --idx, --n) {
        Bucket* bucket = new_collection->arData + idx - 1;
        if (Z_REFCOUNTED(bucket->val))
            GC_ADDREF(Z_COUNTED(bucket->val));
        zend_hash_del_bucket(new_collection, bucket);
    }
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, dropLastWhile)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    ARRAY_CLONE(new_collection, current);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(new_collection, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
            zend_hash_del_bucket(new_collection, bucket);
        else
            break;
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, dropWhile)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    ARRAY_CLONE(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(new_collection, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
            zend_hash_del_bucket(new_collection, bucket);
        else
            break;
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, fill)
{
    zval* element;
    zend_long from_idx = 0;
    zval rv;
    zval* current = COLLECTION_FETCH_EX();
    zend_long num_elements = zend_hash_num_elements(Z_ARRVAL_P(current) - from_idx);
    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_ZVAL(element)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(from_idx)
        Z_PARAM_LONG(num_elements)
    ZEND_PARSE_PARAMETERS_END();
    Bucket* bucket = Z_ARRVAL_P(current)->arData;
    Bucket* end = bucket + Z_ARRVAL_P(current)->nNumUsed;
    for (bucket += from_idx; num_elements > 0 && bucket < end; ++bucket, --num_elements) {
        if (Z_REFCOUNTED(bucket->val))
            GC_ADDREF(Z_COUNTED(bucket->val));
        if (bucket->key)
            zend_hash_update(Z_ARRVAL_P(current), bucket->key, element);
        else
            zend_hash_index_update(Z_ARRVAL_P(current), bucket->h, element);
    }
}

PHP_METHOD(Collection, filter)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
            if (bucket->key)
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            else
                zend_hash_index_add(new_collection, bucket->h, &bucket->val);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, filterNot)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval))
            if (bucket->key)
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            else
                zend_hash_index_add(new_collection, bucket->h, &bucket->val);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, filterNotTo)
{
    zval* dest;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_OBJECT_OF_CLASS(dest, collections_collection_ce)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    zend_array* dest_arr = Z_ARRVAL_P(COLLECTION_FETCH(dest));
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval))
            if (bucket->key)
                zend_hash_add(dest_arr, bucket->key, &bucket->val);
            else
                zend_hash_next_index_insert(dest_arr, &bucket->val);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, filterTo)
{
    zval* dest;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_OBJECT_OF_CLASS(dest, collections_collection_ce)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI();
    zval* current = COLLECTION_FETCH_EX();
    zend_array* dest_arr = Z_ARRVAL_P(COLLECTION_FETCH(dest));
    ZEND_HASH_FOREACH_BUCKET(Z_ARRVAL_P(current), Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
            if (bucket->key)
                zend_hash_add(dest_arr, bucket->key, &bucket->val);
            else
                zend_hash_next_index_insert(dest_arr, &bucket->val);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, find)
{
    
}

PHP_METHOD(Collection, findLast)
{
    
}

PHP_METHOD(Collection, first)
{
    
}

PHP_METHOD(Collection, flatMap)
{
    
}

PHP_METHOD(Collection, flatMapTo)
{
    
}

PHP_METHOD(Collection, flatten)
{
    
}

PHP_METHOD(Collection, fold)
{
    
}

PHP_METHOD(Collection, foldRight)
{
    
}

PHP_METHOD(Collection, forEach)
{
    
}

PHP_METHOD(Collection, get)
{
    
}

PHP_METHOD(Collection, groupBy)
{
    
}

PHP_METHOD(Collection, groupByTo)
{
    
}

PHP_METHOD(Collection, indexOf)
{
    
}

PHP_METHOD(Collection, indexOfFirst)
{
    
}

PHP_METHOD(Collection, indexOfLast)
{
    
}

PHP_METHOD(Collection, init)
{
    zval* elements = NULL;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    if (elements) {
        ELEMENTS_VALIDATE(elements);
        ARRAY_CLONE(new_collection, elements);
        RETURN_NEW_COLLECTION(new_collection);
    }
    ARRAY_NEW(collection, 0);
    RETVAL_NEW_COLLECTION(collection);
}

PHP_METHOD(Collection, intersect)
{
    
}

PHP_METHOD(Collection, isEmpty)
{
    
}

PHP_METHOD(Collection, isNotEmpty)
{
    
}

PHP_METHOD(Collection, keys)
{
    
}

PHP_METHOD(Collection, last)
{
    
}

PHP_METHOD(Collection, lastIndexOf)
{
    
}

PHP_METHOD(Collection, map)
{
    
}

PHP_METHOD(Collection, mapKeys)
{
    
}

PHP_METHOD(Collection, mapKeysTo)
{
    
}

PHP_METHOD(Collection, mapTo)
{
    
}

PHP_METHOD(Collection, mapValues)
{
    
}

PHP_METHOD(Collection, mapValuesTo)
{
    
}

PHP_METHOD(Collection, max)
{
    
}

PHP_METHOD(Collection, maxBy)
{
    
}

PHP_METHOD(Collection, maxWith)
{
    
}

PHP_METHOD(Collection, min)
{
    
}

PHP_METHOD(Collection, minBy)
{
    
}

PHP_METHOD(Collection, minWith)
{
    
}

PHP_METHOD(Collection, minus)
{
    
}

PHP_METHOD(Collection, minusAssign)
{
    
}

PHP_METHOD(Collection, minusKeys)
{
    
}

PHP_METHOD(Collection, minusKeysAssign)
{
    
}

PHP_METHOD(Collection, minusValues)
{
    
}

PHP_METHOD(Collection, minusValuesAssign)
{
    
}

PHP_METHOD(Collection, none)
{
    
}

PHP_METHOD(Collection, onEach)
{
    
}

PHP_METHOD(Collection, orEmpty)
{
    
}

PHP_METHOD(Collection, partition)
{
    
}

PHP_METHOD(Collection, plus)
{
    
}

PHP_METHOD(Collection, plusAssign)
{
    
}

PHP_METHOD(Collection, plusValues)
{
    
}

PHP_METHOD(Collection, plusValuesAssign)
{
    
}

PHP_METHOD(Collection, putAll)
{
    
}

PHP_METHOD(Collection, reduce)
{
    
}

PHP_METHOD(Collection, reduceRight)
{
    
}

PHP_METHOD(Collection, remove)
{
    
}

PHP_METHOD(Collection, removeAll)
{
    
}

PHP_METHOD(Collection, retainAll)
{
    
}

PHP_METHOD(Collection, reverse)
{
    
}

PHP_METHOD(Collection, reversed)
{
    
}

PHP_METHOD(Collection, shuffle)
{
    
}

PHP_METHOD(Collection, single)
{
    
}

PHP_METHOD(Collection, slice)
{
    
}

PHP_METHOD(Collection, sort)
{
    
}

PHP_METHOD(Collection, sortBy)
{
    
}

PHP_METHOD(Collection, sortByDescending)
{
    
}

PHP_METHOD(Collection, sortDescending)
{
    
}

PHP_METHOD(Collection, sortWith)
{
    
}

PHP_METHOD(Collection, sorted)
{
    
}

PHP_METHOD(Collection, sortedBy)
{
    
}

PHP_METHOD(Collection, sortedByDescending)
{
    
}

PHP_METHOD(Collection, sortedDescending)
{
    
}

PHP_METHOD(Collection, sortedWith)
{
    
}

PHP_METHOD(Collection, take)
{
    
}

PHP_METHOD(Collection, takeLast)
{
    
}

PHP_METHOD(Collection, takeLastWhile)
{
    
}

PHP_METHOD(Collection, takeWhile)
{
    
}

PHP_METHOD(Collection, toArray)
{
    zval rv;
    zval* data = COLLECTION_FETCH_EX();
    RETVAL_ZVAL(data, 1, 0);
}

PHP_METHOD(Collection, toCollection)
{
    
}

PHP_METHOD(Collection, toPairs)
{
    
}

PHP_METHOD(Collection, union)
{
    
}

PHP_METHOD(Collection, values)
{
    
}

PHP_METHOD(Pair, __construct)
{
    zval* first;
    zval* second;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(first)
        Z_PARAM_ZVAL(second)
    ZEND_PARSE_PARAMETERS_END();
    PAIR_UPDATE_FIRST(getThis(), first);
    PAIR_UPDATE_SECOND(getThis(), second);
}