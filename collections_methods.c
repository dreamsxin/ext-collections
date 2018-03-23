//
// ext-collections/collections_methods.c
//
// @Author CismonX
//

#include <php.h>

#include "php_collections.h"
#include "php_collections_fe.h"

#define NEW_COLLECTION_OBJ(name) \
    zend_object* (name) = (zend_object*)ecalloc(1, sizeof(zend_object) + \
    zend_object_properties_size(collections_collection_ce)); \
    zend_object_std_init(name, collections_collection_ce); \
    object_properties_init(name, collections_collection_ce); \
    obj->handlers = &std_object_handlers;

#define IS_COLLECTION(zval) \
    Z_TYPE_P(elements) == IS_OBJECT && Z_OBJCE_P(elements) == collections_collection_ce

#define COLLECTION_UPDATE(obj, value) \
    zend_update_property_ex(zend_get_executed_scope(), obj, collection_property_name, value)
#define COLLECTION_UPDATE_EX(value) COLLECTION_UPDATE(getThis(), value)
#define COLLECTION_FETCH(obj) \
    zend_read_property_ex(zend_get_executed_scope(), obj, collection_property_name, 1, &rv)
#define COLLECTION_FETCH_EX() COLLECTION_FETCH(getThis())

#define PHP_COLLECTIONS_ERROR(type, msg) php_error_docref(NULL, type, msg)
#define ERR_BAD_ARGUMENT_TYPE() PHP_COLLECTIONS_ERROR(E_WARNING, "Bad argument type")

PHP_METHOD(Collection, __construct)
{
    
}

PHP_METHOD(Collection, addAll)
{
    
}

PHP_METHOD(Collection, all)
{
    
}

PHP_METHOD(Collection, any)
{
    
}

PHP_METHOD(Collection, associate)
{
    
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
        if (IS_COLLECTION(elements)) {
            zval rv;
            elements = COLLECTION_FETCH(elements);
        } else if (UNEXPECTED(Z_TYPE_P(elements) != IS_ARRAY)) {
            ERR_BAD_ARGUMENT_TYPE();
            RETVAL_NULL();
        }
        COLLECTION_UPDATE(&retval, elements);
    } else {
        zval collection;
        ZVAL_NEW_ARR(&collection);
        zend_hash_init(Z_ARRVAL(collection), 0, NULL, ZVAL_PTR_DTOR, 0);
        COLLECTION_UPDATE(&retval, &collection);
        zval_ptr_dtor(&collection);
    }
    ZVAL_COPY_VALUE(return_value, &retval);
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
    
}