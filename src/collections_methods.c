//
// ext-collections/collections_methods.c
//
// @Author CismonX
//

#include <php.h>

#include "php_collections.h"
#include "php_collections_me.h"

#define NEW_OBJ(name, ce) \
    zend_object* (name) = (zend_object*)ecalloc(1, sizeof(zend_object) + \
    zend_object_properties_size(ce)); \
    zend_object_std_init(name, ce); \
    object_properties_init(name, ce); \
    (name)->handlers = &std_object_handlers;
#define NEW_COLLECTION_OBJ(name) NEW_OBJ(name, collections_collection_ce)
#define NEW_PAIR_OBJ(name) NEW_OBJ(name, collections_pair_ce)

#define IS_COLLECTION_P(zval) \
    Z_TYPE_P(zval) == IS_OBJECT && Z_OBJCE_P(zval) == collections_collection_ce
#define IS_PAIR(zval) \
    Z_TYPE(zval) == IS_OBJECT && Z_OBJCE(zval) == collections_pair_ce

#define OBJ_PROPERTY_UPDATE(obj, property_name, value) \
    zend_update_property_ex(zend_get_executed_scope(), obj, property_name, value)
#define OBJ_PROPERTY_FETCH(obj, property_name) \
    zend_read_property_ex(zend_get_executed_scope(), obj, property_name, 1, &rv)
#define COLLECTION_UPDATE(obj, value) OBJ_PROPERTY_UPDATE(obj, collection_property_name, value)
#define COLLECTION_UPDATE_EX(value) COLLECTION_UPDATE(getThis(), value)
#define COLLECTION_FETCH(obj) OBJ_PROPERTY_FETCH(obj, collection_property_name)
#define COLLECTION_FETCH_EX() COLLECTION_FETCH(getThis())
#define PAIR_UPDATE_FIRST(obj, value) OBJ_PROPERTY_UPDATE(obj, pair_first_name, value)
#define PAIR_UPDATE_SECOND(obj, value) OBJ_PROPERTY_UPDATE(obj, pair_second_name, value)
#define PAIR_FETCH_FIRST(obj) OBJ_PROPERTY_FETCH(obj, pair_first_name)
#define PAIR_FETCH_SECOND(obj) OBJ_PROPERTY_FETCH(obj, pair_second_name)

#define INIT_FCI() \
    zval params[2], rv, retval; \
    fci.param_count = 2; \
    fci.retval = &retval; \
    fci.params = params;

#define CALLBACK_PASS_PAIR(bucket) \
    ZVAL_COPY(&params[0], &(bucket)->val); \
    if ((bucket)->key) \
        ZVAL_STR(&params[1], (bucket)->key); \
    else \
        ZVAL_LONG(&params[1], (bucket)->h)

#define PHP_COLLECTIONS_ERROR(type, msg) php_error_docref(NULL, type, msg)
#define ERR_BAD_ARGUMENT_TYPE() PHP_COLLECTIONS_ERROR(E_WARNING, "Bad argument type")
#define ERR_BAD_KEY_TYPE() PHP_COLLECTIONS_ERROR(E_WARNING, "Key must be integer or string")
#define ERR_BAD_CALLBACK_RETVAL() PHP_COLLECTIONS_ERROR(E_WARNING, "Bad callback return value")

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

#define RETVAL_NEW_COLLECTION(collection) \
    do { \
        NEW_COLLECTION_OBJ(obj); \
        zval _retval; \
        ZVAL_OBJ(&_retval, obj); \
        zval _property; \
        ZVAL_ARR(&_property, collection); \
        COLLECTION_UPDATE(&_retval, &_property); \
        zval_ptr_dtor(&_property); \
        RETVAL_OBJ(obj); \
    } while (0)

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
    ZEND_HASH_FOREACH_VAL_IND(Z_ARRVAL_P(elements), zval* val)
        zend_hash_next_index_insert(Z_ARRVAL_P(current), val);
    ZEND_HASH_FOREACH_END();
    COLLECTION_UPDATE_EX(current);
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
        ZVAL_COPY_VALUE(&params[0], &bucket->val);
        CALLBACK_PASS_PAIR(bucket);
        zend_call_function(&fci, &fcc);
        zval_ptr_dtor(&params[0]);
        if (Z_TYPE(retval) == IS_FALSE)
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
        ZVAL_COPY_VALUE(&params[0], &bucket->val);
        CALLBACK_PASS_PAIR(bucket);
        zend_call_function(&fci, &fcc);
        zval_ptr_dtor(&params[0]);
        if (Z_TYPE(retval) == IS_TRUE)
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
        ZVAL_COPY_VALUE(&params[0], &bucket->val);
        CALLBACK_PASS_PAIR(bucket);
        zend_call_function(&fci, &fcc);
        zval_ptr_dtor(&params[0]);
        if (EXPECTED(IS_PAIR(retval))) {
            zval* key = PAIR_FETCH_FIRST(&retval);
            zval* value = PAIR_FETCH_SECOND(&retval);
            if (Z_TYPE_P(key) == IS_LONG)
                zend_hash_index_add(new_collection, Z_LVAL_P(key), value);
            else if (Z_TYPE_P(key) == IS_STRING)
                zend_hash_add(new_collection, Z_STR_P(key), value);
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
    
}

PHP_METHOD(Collection, associateBy)
{
    
}

PHP_METHOD(Collection, associateByTo)
{
    
}

PHP_METHOD(Collection, average)
{
    
}

PHP_METHOD(Collection, containsAll)
{
    
}

PHP_METHOD(Collection, containsKey)
{
    
}

PHP_METHOD(Collection, containsValue)
{
    
}

PHP_METHOD(Collection, copyOf)
{
    
}

PHP_METHOD(Collection, copyOfRange)
{
    
}

PHP_METHOD(Collection, count)
{
    
}

PHP_METHOD(Collection, distinct)
{
    
}

PHP_METHOD(Collection, distinctBy)
{
    
}

PHP_METHOD(Collection, drop)
{
    
}

PHP_METHOD(Collection, dropLast)
{
    
}

PHP_METHOD(Collection, dropLastWhile)
{
    
}

PHP_METHOD(Collection, dropWhile)
{
    
}

PHP_METHOD(Collection, fill)
{
    
}

PHP_METHOD(Collection, filter)
{
    
}

PHP_METHOD(Collection, filterNot)
{
    
}

PHP_METHOD(Collection, filterNotTo)
{
    
}

PHP_METHOD(Collection, filterTo)
{
    
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
    NEW_COLLECTION_OBJ(obj);
    zval retval;
    ZVAL_OBJ(&retval, obj);
    if (elements) {
        ELEMENTS_VALIDATE(elements);
        COLLECTION_UPDATE(&retval, elements);
    } else {
        zval collection;
        ZVAL_NEW_ARR(&collection);
        zend_hash_init(Z_ARRVAL(collection), 0, NULL, ZVAL_PTR_DTOR, 0);
        COLLECTION_UPDATE(&retval, &collection);
        zval_ptr_dtor(&collection);
    }
    RETVAL_OBJ(obj);
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

PHP_METHOD(Collection, offsetUnset)
{
    
}

PHP_METHOD(Collection, offsetSet)
{
    
}

PHP_METHOD(Collection, offsetGet)
{
    
}

PHP_METHOD(Collection, offsetExists)
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

PHP_METHOD(Collection, sliceRange)
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