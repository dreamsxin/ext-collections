//
// ext-collections/collections_methods.c
//
// @Author CismonX
//

#include <php.h>
#include <ext/standard/php_string.h>
#include <ext/standard/php_mt_rand.h>

#include "php_collections.h"
#include "php_collections_me.h"

#define NEW_OBJ(name, ce, object_handlers)                                 \
    zend_object* (name) = (zend_object*)ecalloc(1, sizeof(zend_object) +   \
        zend_object_properties_size(ce));                                  \
    zend_object_std_init(name, ce);                                        \
    (name)->handlers = object_handlers
#define NEW_COLLECTION_OBJ(name)                                           \
    NEW_OBJ(name, collections_collection_ce, &collection_handlers)
#define NEW_PAIR_OBJ(name)                                                 \
    NEW_OBJ(name, collections_pair_ce, &std_object_handlers);              \
    name->properties = (zend_array*)emalloc(sizeof(zend_array));           \
    zend_hash_init(name->properties, 2, NULL, ZVAL_PTR_DTOR, 0)

#define IS_COLLECTION_P(zval)                                              \
    Z_TYPE_P(zval) == IS_OBJECT && Z_OBJCE_P(zval) == collections_collection_ce
#define IS_PAIR(zval)                                                      \
    EXPECTED(Z_TYPE(zval) == IS_OBJECT) && EXPECTED(Z_OBJCE(zval) == collections_pair_ce)

#define OBJ_PROPERTY_UPDATE(obj, property_name, value)                     \
    zend_hash_update((obj)->properties, property_name, value)
#define OBJ_PROPERTY_FETCH(obj, property_name)                             \
    zend_hash_find((obj)->properties, property_name)
#define PAIR_UPDATE_FIRST(obj, value) OBJ_PROPERTY_UPDATE(obj, collections_pair_first, value)
#define PAIR_UPDATE_SECOND(obj, value) OBJ_PROPERTY_UPDATE(obj, collections_pair_second, value)
#define PAIR_FETCH_FIRST(obj) OBJ_PROPERTY_FETCH(obj, collections_pair_first)
#define PAIR_FETCH_SECOND(obj)  OBJ_PROPERTY_FETCH(obj, collections_pair_second)
#define COLLECTION_FETCH(obj) Z_OBJ_P(obj)->properties
#define COLLECTION_FETCH_CURRENT() COLLECTION_FETCH(getThis())

#define SEPARATE_COLLECTION(ht, obj)                                       \
    if (GC_REFCOUNT(ht) > 1)                                               \
    {                                                                      \
        GC_DELREF(ht);                                                     \
        ht = Z_OBJ_P(obj)->properties = zend_array_dup(ht);                \
    }
#define SEPARATE_CURRENT_COLLECTION(ht) SEPARATE_COLLECTION(ht, getThis())

#define INIT_FCI(fci, num_args)                                            \
    zval params[num_args], retval;                                         \
    (fci)->size = sizeof(zend_fcall_info);                                 \
    (fci)->param_count = num_args;                                         \
    (fci)->retval = &retval;                                               \
    (fci)->params = params

#define CALLBACK_KEYVAL_INVOKE(params, bucket)                             \
    ZVAL_COPY_VALUE(&params[0], &bucket->val);                             \
    if ((bucket)->key)                                                     \
    {                                                                      \
        ZVAL_STR(&params[1], (bucket)->key);                               \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        ZVAL_LONG(&params[1], (bucket)->h);                                \
    }                                                                      \
    zend_call_function(&fci, &fcc)

#define PHP_COLLECTIONS_ERROR(type, msg) php_error_docref(NULL, type, msg)
#define ERR_BAD_ARGUMENT_TYPE() PHP_COLLECTIONS_ERROR(E_WARNING, "Bad argument type")
#define ERR_BAD_KEY_TYPE() PHP_COLLECTIONS_ERROR(E_WARNING, "Key must be integer or string")
#define ERR_BAD_CALLBACK_RETVAL() PHP_COLLECTIONS_ERROR(E_WARNING, "Bad callback return value")
#define ERR_BAD_SIZE() PHP_COLLECTIONS_ERROR(E_WARNING, "Size must be non-negative")
#define ERR_BAD_INDEX() PHP_COLLECTIONS_ERROR(E_WARNING, "Index must be non-negative")
#define ERR_NOT_ARITHMETIC() PHP_COLLECTIONS_ERROR(E_WARNING, "Elements should be int or double")
#define ERR_SILENCED()

#define ELEMENTS_VALIDATE(elements, err, err_then)                         \
    zend_array* elements##_arr;                                            \
    if (IS_COLLECTION_P(elements))                                         \
    {                                                                      \
        (elements##_arr) = COLLECTION_FETCH(elements);                     \
    }                                                                      \
    else if (UNEXPECTED(Z_TYPE_P(elements) == IS_ARRAY))                   \
    {                                                                      \
        (elements##_arr) = Z_ARRVAL_P(elements);                           \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        err();                                                             \
        err_then;                                                          \
    }

#define ARRAY_NEW(name, size)                                              \
    zend_array* (name) = (zend_array*)emalloc(sizeof(zend_array));         \
    zend_hash_init(name, size, NULL, ZVAL_PTR_DTOR, 0)
#define ARRAY_NEW_EX(name, other)                                          \
    ARRAY_NEW(name, zend_hash_num_elements(other))
#define ARRAY_CLONE(dest, src)                                             \
    ARRAY_NEW_EX(dest, src);                                               \
    zend_hash_copy(dest, src, NULL)

#define RETVAL_NEW_COLLECTION(collection)                                  \
    do                                                                     \
    {                                                                      \
        NEW_COLLECTION_OBJ(obj);                                           \
        if (GC_REFCOUNT(collection) > 1)                                   \
            GC_ADDREF(collection);                                         \
        obj->properties = collection;                                      \
        RETVAL_OBJ(obj);                                                   \
    } while (0)

#define RETURN_NEW_COLLECTION(collection)                                  \
    {                                                                      \
        RETVAL_NEW_COLLECTION(collection);                                 \
        return;                                                            \
    }

#define FCI_G    COLLECTIONS_G(fci)
#define FCC_G    COLLECTIONS_G(fcc)

typedef int (*equal_check_func_t)(zval*, zval*);

/// Unused global variable.
zval rv;

static zend_always_inline void bucket_to_pair(zend_object* pair, Bucket* bucket)
{
    zval key;
    if (bucket->key)
    {
        GC_ADDREF(bucket->key);
        ZVAL_STR(&key, bucket->key);
    }
    else
    {
        ZVAL_LONG(&key, bucket->h);
    }
    PAIR_UPDATE_FIRST(pair, &key);
    PAIR_UPDATE_SECOND(pair, &bucket->val);
}

static zend_always_inline int bucket_compare_numeric(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    return numeric_compare_function(&b1->val, &b2->val);
}

static int bucket_reverse_compare_numeric(const void* op1, const void* op2)
{
    return bucket_compare_numeric(op2, op1);
}

static zend_always_inline int bucket_compare_string_ci(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    return string_case_compare_function(&b1->val, &b2->val);
}

static int bucket_reverse_compare_string_ci(const void* op1, const void* op2)
{
    return bucket_compare_string_ci(op2, op1);
}

static int zend_always_inline bucket_compare_string_cs(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    return string_compare_function(&b1->val, &b2->val);
}

static int bucket_reverse_compare_string_cs(const void* op1, const void* op2)
{
    return bucket_compare_string_cs(op2, op1);
}

static zend_always_inline int bucket_compare_natural_ci(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    zval* v1 = &b1->val;
    zval* v2 = &b2->val;
    return strnatcmp_ex(Z_STRVAL_P(v1), Z_STRLEN_P(v1), Z_STRVAL_P(v2), Z_STRLEN_P(v2), 1);
}

static int bucket_reverse_compare_natural_ci(const void* op1, const void* op2)
{
    return bucket_compare_natural_ci(op2, op1);
}

static zend_always_inline int bucket_compare_natural_cs(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    zval* v1 = &b1->val;
    zval* v2 = &b2->val;
    return strnatcmp_ex(Z_STRVAL_P(v1), Z_STRLEN_P(v1), Z_STRVAL_P(v2), Z_STRLEN_P(v2), 0);
}

static int bucket_reverse_compare_natural_cs(const void* op1, const void* op2)
{
    return bucket_compare_natural_cs(op2, op1);
}

static zend_always_inline int bucket_compare_regular(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    zval result;
    if (compare_function(&result, &b1->val, &b2->val) == FAILURE)
    {
        return 0;
    }
    return ZEND_NORMALIZE_BOOL(Z_LVAL(result));
}

static int bucket_reverse_compare_regular(const void* op1, const void* op2)
{
    return bucket_compare_regular(op2, op1);
}

static int bucket_compare_userland(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    INIT_FCI(FCI_G, 2);
    ZVAL_COPY_VALUE(&params[0], &b1->val);
    ZVAL_COPY_VALUE(&params[1], &b2->val);
    zend_call_function(FCI_G, FCC_G);
    int result = ZEND_NORMALIZE_BOOL(zval_get_long(&retval));
    zval_ptr_dtor(&retval);
    return result;
}

static zend_always_inline equal_check_func_t equal_check_func_init(zval* val)
{
    if (Z_TYPE_P(val) == IS_LONG)
    {
        return fast_equal_check_long;
    }
    if (Z_TYPE_P(val) == IS_STRING)
    {
        return fast_equal_check_string;
    } 
    return fast_equal_check_function;
}

static zend_always_inline compare_func_t compare_func_init(
    zval* val, zend_bool reverse, zend_long flags)
{
    zend_bool case_insensitive = flags & PHP_COLLECTIONS_FOLD_CASE;
    if (Z_TYPE_P(val) == IS_LONG || Z_TYPE_P(val) == IS_DOUBLE)
    {
        return reverse ? bucket_reverse_compare_numeric : bucket_compare_numeric;
    }
    if (Z_TYPE_P(val) == IS_STRING)
    {
        if ((flags & ~PHP_COLLECTIONS_FOLD_CASE) == PHP_COLLECTIONS_COMPARE_NATURAL)
        {
            if (case_insensitive)
            {
                return reverse ? bucket_reverse_compare_natural_ci : bucket_compare_natural_ci;
            }
            return reverse ? bucket_reverse_compare_natural_cs : bucket_compare_natural_cs;
        }
        if (case_insensitive)
        {
            return reverse ? bucket_reverse_compare_string_ci : bucket_compare_string_ci;
        }
        return reverse ? bucket_reverse_compare_string_cs : bucket_compare_string_cs;
    }
    return reverse ? bucket_reverse_compare_regular : bucket_compare_regular;
}

int count_collection(zval* obj, zend_long* count)
{
    zend_array* current = COLLECTION_FETCH(obj);
    *count = zend_hash_num_elements(current);
    return SUCCESS;
}

int collection_offset_exists(zval* object, zval* offset, int check_empty)
{
    zend_array* current = COLLECTION_FETCH(object);
    if (check_empty)
    {
        return zend_hash_num_elements(current) == 0;
    }
    if (Z_TYPE_P(offset) == IS_LONG)
    {
        return zend_hash_index_exists(current, Z_LVAL_P(offset));
    }
    if (Z_TYPE_P(offset) == IS_STRING)
    {
        return zend_hash_exists(current, Z_STR_P(offset));
    }
    return 0;
}

void collection_offset_set(zval* object, zval* offset, zval* value)
{
    zend_array* current = COLLECTION_FETCH(object);
    SEPARATE_COLLECTION(current, object);
    if (Z_TYPE_P(offset) == IS_LONG)
    {
        zend_hash_index_update(current, Z_LVAL_P(offset), value);
    }
    else if (Z_TYPE_P(offset) == IS_STRING)
    {
        zend_hash_update(current, Z_STR_P(offset), value);
    }
}

zval* collection_offset_get(zval* object, zval* offset, int type, zval* retval)
{
    // Note that we don't handle type. So don't do any fancy things with Collection
    // such as fetching a reference of a value, etc.
    zend_array* current = COLLECTION_FETCH(object);
    zval* found = NULL;
    if (Z_TYPE_P(offset) == IS_LONG)
    {
        found = zend_hash_index_find(current, Z_LVAL_P(offset));
    }
    else if (Z_TYPE_P(offset) == IS_STRING)
    {
        found = zend_hash_find(current, Z_STR_P(offset));
    }
    ZVAL_COPY(retval, found);
    return retval;
}

void collection_offset_unset(zval* object, zval* offset)
{
    zend_array* current = COLLECTION_FETCH(object);
    SEPARATE_COLLECTION(current, object);
    if (Z_TYPE_P(offset) == IS_LONG)
    {
        zend_hash_index_del(current, Z_LVAL_P(offset));
    }
    else if (Z_TYPE_P(offset) == IS_STRING)
    {
        zend_hash_del(current, Z_STR_P(offset));
    }
}

PHP_METHOD(Collection, __construct) {}

PHP_METHOD(Collection, addAll)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    SEPARATE_CURRENT_COLLECTION(current);
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key)
        {
            zend_hash_add(current, bucket->key, &bucket->val);
        }
        else if (HT_IS_PACKED(elements_arr))
        {
            zend_hash_next_index_insert(current, &bucket->val);
        }
        else
        {
            zend_hash_index_add(current, bucket->h, &bucket->val);
        }
    ZEND_HASH_FOREACH_END();
}

PHP_METHOD(Collection, all)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            RETURN_FALSE;
        }
        zval_ptr_dtor(&retval);
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
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            RETURN_TRUE;
        }
        zval_ptr_dtor(&retval);
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
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (IS_PAIR(retval))
        {
            zval* key = PAIR_FETCH_FIRST(Z_OBJ(retval));
            zval* value = PAIR_FETCH_SECOND(Z_OBJ(retval));
            if (Z_TYPE_P(key) == IS_LONG)
            {
                zend_hash_index_add(new_collection, Z_LVAL_P(key), value);
            }
            else if (Z_TYPE_P(key) == IS_STRING)
            {
                zend_hash_add(new_collection, Z_STR_P(key), value);
            }
            else if (Z_TYPE_P(key) == IS_NULL)
            {
                zend_hash_next_index_insert(new_collection, value);
            }
            else
            {
                ERR_BAD_KEY_TYPE();
            }
        }
        else
        {
            ERR_BAD_CALLBACK_RETVAL();
        }
        zval_ptr_dtor(&retval);
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
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* dest_arr = COLLECTION_FETCH(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (IS_PAIR(retval))
        {
            zval* key = PAIR_FETCH_FIRST(Z_OBJ(retval));
            zval* value = PAIR_FETCH_SECOND(Z_OBJ(retval));
            if (Z_TYPE_P(key) == IS_LONG)
            {
                zend_hash_index_add(dest_arr, Z_LVAL_P(key), value);
            }
            else if (Z_TYPE_P(key) == IS_STRING)
            {
                zend_hash_add(dest_arr, Z_STR_P(key), value);
            }
            else if (Z_TYPE_P(key) == IS_NULL)
            {
                zend_hash_next_index_insert(dest_arr, value);
            }
            else
            {
                ERR_BAD_KEY_TYPE();
            }
        }
        else
        {
            ERR_BAD_CALLBACK_RETVAL();
        }
        zval_ptr_dtor(&retval);
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
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (Z_TYPE(retval) == IS_LONG)
        {
            zend_hash_index_add(new_collection, Z_LVAL(retval), &bucket->val);
        }
        else if (Z_TYPE(retval) == IS_STRING)
        {
            zend_hash_add(new_collection, Z_STR(retval), &bucket->val);
        }
        else
        {
            ERR_BAD_CALLBACK_RETVAL();
        }
        zval_ptr_dtor(&retval);
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
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* dest_arr = COLLECTION_FETCH(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (Z_TYPE(retval) == IS_LONG)
        {
            zend_hash_index_add(dest_arr, Z_LVAL(retval), &bucket->val);
        }
        else if (Z_TYPE(retval) == IS_STRING)
        {
            zend_hash_add(dest_arr, Z_STR(retval), &bucket->val);
        }
        else
        {
            ERR_BAD_CALLBACK_RETVAL();
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, average)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    double sum = 0;
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        if (Z_TYPE_P(val) == IS_LONG)
        {
            sum += Z_LVAL_P(val);
        }
        else if (Z_TYPE_P(val) == IS_DOUBLE)
        {
            sum += Z_DVAL_P(val);
        }
        else
        {
            ERR_NOT_ARITHMETIC();
            return;
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_DOUBLE(sum / zend_hash_num_elements(current));
}

PHP_METHOD(Collection, containsAll)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ZEND_HASH_FOREACH_VAL(elements_arr, zval* element)
        equal_check_func_t eql = equal_check_func_init(element);
        int result = 0;
        ZEND_HASH_FOREACH_VAL(current, zval* val)
            result = eql(element, val);
            if (result)
            {
                break;
            }
        ZEND_HASH_FOREACH_END();
        if (result == 0)
        {
            RETURN_FALSE;
        }
    ZEND_HASH_FOREACH_END();
    RETURN_TRUE;
}

PHP_METHOD(Collection, containsKey)
{
    zval* key;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(key)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    if (Z_TYPE_P(key) == IS_LONG)
    {
        RETURN_BOOL(zend_hash_index_exists(current, Z_LVAL_P(key)));
    }
    if (Z_TYPE_P(key) == IS_STRING)
    {
        RETURN_BOOL(zend_hash_exists(current, Z_STR_P(key)));
    }
    ERR_BAD_KEY_TYPE();
    RETVAL_FALSE;
}

PHP_METHOD(Collection, containsValue)
{
    zval* element;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(element)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    equal_check_func_t eql = equal_check_func_init(element);
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        if (eql(element, val))
        {
            RETURN_TRUE;
        }
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
    zend_array* current = COLLECTION_FETCH_CURRENT();
    if (EX_NUM_ARGS() == 0)
    {
        ARRAY_CLONE(new_collection, current);
        RETURN_NEW_COLLECTION(new_collection);
    }
    if (UNEXPECTED(new_size < 0))
    {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    if (new_size == 0)
    {
        ARRAY_NEW(new_collection, 0);
        RETURN_NEW_COLLECTION(new_collection);
    }
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        if (bucket->key)
        {
            zend_hash_add_new(new_collection, bucket->key, &bucket->val);
        }
        else
        {
            zend_hash_index_add_new(new_collection, bucket->h, &bucket->val);
        }
        if (--new_size == 0)
        {
            break;
        }
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
    if (from_idx < 0)
    {
        ERR_BAD_INDEX();
        RETURN_NULL();
    }
    if (num_elements < 0)
    {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW(new_collection, num_elements);
    Bucket* bucket = current->arData;
    Bucket* end = bucket + current->nNumUsed;
    for (bucket += from_idx; num_elements > 0 && bucket < end; ++bucket) {
        if (Z_ISUNDEF(bucket->val))
        {
            continue;
        }
        --num_elements;
        if (bucket->key)
        {
            zend_hash_add(new_collection, bucket->key, &bucket->val);
        }
        else if (HT_IS_PACKED(current))
        {
            zend_hash_next_index_insert(new_collection, &bucket->val);
        }
        else
        {
            zend_hash_index_add(new_collection, bucket->h, &bucket->val);
        }
    }
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
    if (n < 0)
    {
        ERR_BAD_SIZE();
        return;
    }
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_CLONE(new_collection, current);
    Bucket* bucket = new_collection->arData;
    Bucket* end = bucket + new_collection->nNumUsed;
    for (; n > 0 && bucket < end; ++bucket)
    {
        if (Z_ISUNDEF(bucket->val))
        {
            continue;
        }
        --n;
        Z_TRY_ADDREF(bucket->val);
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
    if (n < 0)
    {
        ERR_BAD_SIZE();
        return;
    }
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_CLONE(new_collection, current);
    unsigned idx = new_collection->nNumUsed;
    for (; n > 0 && idx > 0; --idx)
    {
        Bucket* bucket = new_collection->arData + idx - 1;
        if (Z_ISUNDEF(bucket->val))
        {
            continue;
        }
        --n;
        Z_TRY_ADDREF(bucket->val);
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
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_CLONE(new_collection, current);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(new_collection, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            zend_hash_del_bucket(new_collection, bucket);
        }
        else
        {
            zval_ptr_dtor(&retval);
            break;
        }
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
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_CLONE(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(new_collection, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            zend_hash_del_bucket(new_collection, bucket);
        }
        else
        {
            zval_ptr_dtor(&retval);
            break;
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, fill)
{
    zval* element;
    zend_long from_idx = 0;
    zend_array* current = COLLECTION_FETCH_CURRENT();
    SEPARATE_CURRENT_COLLECTION(current);
    zend_long num_elements = zend_hash_num_elements(current - from_idx);
    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_ZVAL(element)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(from_idx)
        Z_PARAM_LONG(num_elements)
    ZEND_PARSE_PARAMETERS_END();
    Bucket* bucket = current->arData;
    Bucket* end = bucket + current->nNumUsed;
    for (bucket += from_idx; num_elements > 0 && bucket < end; ++bucket, --num_elements)
    {
        if (Z_ISUNDEF(bucket->val))
        {
            continue;
        }
        if (bucket->key)
        {
            zend_hash_update(current, bucket->key, element);
        }
        else
        {
            zend_hash_index_update(current, bucket->h, element);
        }
    }
}

PHP_METHOD(Collection, filter)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            if (bucket->key)
            {
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            }
            else if (HT_IS_PACKED(current))
            {
                zend_hash_next_index_insert(new_collection, &bucket->val);
            }
            else
            {
                zend_hash_index_add(new_collection, bucket->h, &bucket->val);
            }
        }
        zval_ptr_dtor(&retval);
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
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval))
        {
            if (bucket->key)
            {
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            }
            else if (HT_IS_PACKED(current))
            {
                zend_hash_next_index_insert(new_collection, &bucket->val);
            }
            else
            {
                zend_hash_index_add(new_collection, bucket->h, &bucket->val);
            }
        }
        zval_ptr_dtor(&retval);
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
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* dest_arr = COLLECTION_FETCH(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval))
        {
            if (bucket->key)
            {
                zend_hash_add(dest_arr, bucket->key, &bucket->val);
            }
            else if (HT_IS_PACKED(current))
            {
                zend_hash_next_index_insert(dest_arr, &bucket->val);
            }
            else
            {
                zend_hash_index_add(dest_arr, bucket->h, &bucket->val);
            }
        }
        zval_ptr_dtor(&retval);
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
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* dest_arr = COLLECTION_FETCH(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            if (bucket->key)
            {
                zend_hash_add(dest_arr, bucket->key, &bucket->val);
            }
            else if (HT_IS_PACKED(current))
            {
                zend_hash_next_index_insert(dest_arr, &bucket->val);
            }
            else
            {
                zend_hash_index_add(dest_arr, bucket->h, &bucket->val);
            }
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, first)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    if (zend_hash_num_elements(current) == 0)
    {
        RETURN_NULL();
    }
    if (EX_NUM_ARGS() == 0)
    {
        HashPosition pos = 0;
        while (pos < current->nNumUsed && Z_ISUNDEF(current->arData[pos].val))
        {
            ++pos;
        }
        RETURN_ZVAL(&current->arData[pos].val, 1, 0);
    }
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            RETURN_ZVAL(&bucket->val, 1, 0);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_NULL();
}

PHP_METHOD(Collection, flatMap)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zval* retval_p = &retval;
        ELEMENTS_VALIDATE(retval_p, ERR_BAD_CALLBACK_RETVAL, continue);
        ZEND_HASH_FOREACH_BUCKET(retval_p_arr, Bucket* bucket)
            Z_TRY_ADDREF(bucket->val);
            if (bucket->key)
            {
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            }
            else
            {
                zend_hash_next_index_insert(new_collection, &bucket->val);
            }
        ZEND_HASH_FOREACH_END();
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, flatMapTo)
{
    zval* dest;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_OBJECT_OF_CLASS(dest, collections_collection_ce)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* dest_arr = COLLECTION_FETCH(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zval* retval_p = &retval;
        ELEMENTS_VALIDATE(retval_p, ERR_BAD_CALLBACK_RETVAL, continue);
        ZEND_HASH_FOREACH_BUCKET(retval_p_arr, Bucket* bucket)
            Z_TRY_ADDREF(bucket->val);
            if (bucket->key)
            {
                zend_hash_add(dest_arr, bucket->key, &bucket->val);
            }
            else
            {
                zend_hash_next_index_insert(dest_arr, &bucket->val);
            }
        ZEND_HASH_FOREACH_END();
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, flatten)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        zval* val = &bucket->val;
        ELEMENTS_VALIDATE(val, ERR_SILENCED,
        {
            if (bucket->key)
            {
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            }
            else
            {
                zend_hash_next_index_insert(new_collection, &bucket->val);
            }
            continue;
        });
        ZEND_HASH_FOREACH_BUCKET(val_arr, Bucket* bucket)
            if (bucket->key)
            {
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            }
            else
            {
                zend_hash_next_index_insert(new_collection, &bucket->val);
            }
        ZEND_HASH_FOREACH_END();
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, fold)
{
    zval* initial;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(initial)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    INIT_FCI(&fci, 3);
    ZVAL_COPY(&params[0], initial);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        ZVAL_COPY_VALUE(&params[1], &bucket->val);
        if (bucket->key)
        {
            ZVAL_STR(&params[2], bucket->key);
        }
        else
        {
            ZVAL_LONG(&params[2], bucket->h);
        }
        zend_call_function(&fci, &fcc);
        zval_ptr_dtor(&params[0]);
        ZVAL_COPY_VALUE(&params[0], &retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(&retval, 0, 0);
}

PHP_METHOD(Collection, foldRight)
{
    zval* initial;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(initial)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    INIT_FCI(&fci, 3);
    ZVAL_COPY(&params[0], initial);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        ZVAL_COPY_VALUE(&params[1], &bucket->val);
        if (bucket->key)
        {
            ZVAL_STR(&params[2], bucket->key);
        }
        else
        {
            ZVAL_LONG(&params[2], bucket->h);
        }
        zend_call_function(&fci, &fcc);
        zval_ptr_dtor(&params[0]);
        ZVAL_COPY_VALUE(&params[0], &retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(&retval, 0, 0);
}

PHP_METHOD(Collection, forEach)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
}

PHP_METHOD(Collection, get)
{
    zval* key;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zval* found = NULL;
    if (Z_TYPE_P(key) == IS_STRING)
    {
        found = zend_hash_find(current, Z_STR_P(key));
    }
    else if (Z_TYPE_P(key) == IS_LONG)
    {
        found = zend_hash_index_find(current, Z_LVAL_P(key));
    }
    else
    {
        ERR_BAD_KEY_TYPE();
        RETURN_NULL();
    }
    if (found)
    {
        RETURN_ZVAL(found, 1, 0);
    }
    if (EX_NUM_ARGS() < 2)
    {
        RETURN_NULL();
    }
    INIT_FCI(&fci, 1);
    ZVAL_COPY_VALUE(&params[0], key);
    zend_call_function(&fci, &fcc);
    RETVAL_ZVAL(&retval, 0, 0);
}

PHP_METHOD(Collection, groupBy)
{
    
}

PHP_METHOD(Collection, groupByTo)
{
    
}

PHP_METHOD(Collection, indexOf)
{
    zval* element;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(element)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    equal_check_func_t eql = equal_check_func_init(element);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        if (eql(element, &bucket->val))
        {
            if (bucket->key)
            {
                RETURN_STR(bucket->key);
            }
            RETURN_LONG(bucket->h);
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NULL();
}

PHP_METHOD(Collection, indexOfFirst)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 1);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        ZVAL_COPY_VALUE(&params[0], &bucket->val);
        zend_call_function(&fci, &fcc);
        if (zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            if (bucket->key)
            {
                RETURN_STR(bucket->key);
            }
            RETURN_LONG(bucket->h);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_NULL();
}

PHP_METHOD(Collection, indexOfLast)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 1);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        ZVAL_COPY_VALUE(&params[0], &bucket->val);
        zend_call_function(&fci, &fcc);
        if (zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            if (bucket->key)
            {
                RETURN_STR(bucket->key);
            }
            RETURN_LONG(bucket->h);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_NULL();
}

PHP_METHOD(Collection, init)
{
    zval* elements = NULL;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    if (elements)
    {
        ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
        RETURN_NEW_COLLECTION(elements_arr);
    }
    ARRAY_NEW(collection, 0);
    RETVAL_NEW_COLLECTION(collection);
}

PHP_METHOD(Collection, intersect)
{
    
}

PHP_METHOD(Collection, isEmpty)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    RETVAL_BOOL(zend_hash_num_elements(current) == 0);
}

PHP_METHOD(Collection, isNotEmpty)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    RETVAL_BOOL(zend_hash_num_elements(current));
}

PHP_METHOD(Collection, keys)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        zval val;
        if (bucket->key)
        {
            ZVAL_STR(&val, bucket->key);
        }
        else
        {
            ZVAL_LONG(&val, bucket->h);
        }
        zend_hash_next_index_insert(new_collection, &val);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, last)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    if (zend_hash_num_elements(current) == 0)
    {
        RETURN_NULL();
    }
    if (EX_NUM_ARGS() == 0)
    {
        HashPosition pos = current->nNumUsed;
        while (pos <= current->nNumUsed && Z_ISUNDEF(current->arData[pos].val))
        {
            --pos;
        }
        RETURN_ZVAL(&current->arData[pos].val, 1, 0);
    }
    INIT_FCI(&fci, 2);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            RETURN_ZVAL(&bucket->val, 1, 0);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_NULL();
}

PHP_METHOD(Collection, lastIndexOf)
{
    zval* element;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(element)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    equal_check_func_t eql = equal_check_func_init(element);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        if (eql(element, &bucket->val))
        {
            if (bucket->key)
            {
                RETURN_STR(bucket->key);
            }
            RETURN_LONG(bucket->h);
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NULL();
}

PHP_METHOD(Collection, map)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zend_hash_next_index_insert(new_collection, &retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, mapTo)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    zval* dest;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_OBJECT_OF_CLASS(dest, collections_collection_ce)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* dest_arr = COLLECTION_FETCH(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zend_hash_next_index_insert(dest_arr, &retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, max)
{
    zend_long flags = 0;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    compare_func_t cmp;
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        cmp = compare_func_init(val, 0, flags);
        break;
    ZEND_HASH_FOREACH_END();
    zval* max = zend_hash_minmax(current, cmp, 1);
    if (max)
    {
        RETURN_ZVAL(max, 0, 0);
    }
    RETVAL_NULL();
}

PHP_METHOD(Collection, maxBy)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    zend_long flags = 0;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_FUNC(fci, fcc)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(max_by, current);
    compare_func_t cmp = NULL;
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (UNEXPECTED(cmp == NULL))
        {
            cmp = compare_func_init(&retval, 0, flags);
        }
        zend_hash_index_add(max_by, bucket - current->arData, &retval);
    ZEND_HASH_FOREACH_END();
    zval* max = zend_hash_minmax(max_by, cmp, 1);
    if (max)
    {
        zend_ulong offset = *(zend_ulong*)(max + 1);
        zval* ret = &(current->arData + offset)->val;
        RETVAL_ZVAL(ret, 1, 0);
    }
    else
    {
        RETVAL_NULL();
    }
    zend_hash_destroy(max_by);
    efree(max_by);
}

PHP_METHOD(Collection, maxWith)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    FCI_G = &fci;
    FCC_G = &fcc;
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* max_with = zend_array_dup(current);
    ZEND_HASH_FOREACH_BUCKET(max_with, Bucket* bucket)
        NEW_PAIR_OBJ(obj);
        bucket_to_pair(obj, bucket);
        ZVAL_OBJ(&bucket->val, obj);
    ZEND_HASH_FOREACH_END();
    zval* max = PAIR_FETCH_SECOND(Z_OBJ_P(zend_hash_minmax(max_with, bucket_compare_userland, 1)));
    Z_TRY_ADDREF_P(max);
    zend_array_destroy(max_with);
    if (max)
    {
        RETURN_ZVAL(max, 0, 0);
    }
    RETVAL_NULL();
}

PHP_METHOD(Collection, min)
{
    zend_long flags = 0;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    compare_func_t cmp;
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        cmp = compare_func_init(val, 0, flags);
        break;
    ZEND_HASH_FOREACH_END();
    zval* min = zend_hash_minmax(current, cmp, 0);
    if (min)
    {
        RETURN_ZVAL(min, 0, 0);
    }
    RETVAL_NULL();
}

PHP_METHOD(Collection, minBy)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    zend_long flags = 0;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_FUNC(fci, fcc)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(min_by, current);
    compare_func_t cmp = NULL;
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (UNEXPECTED(cmp == NULL))
        {
            cmp = compare_func_init(&retval, 0, flags);
        }
        zend_hash_index_add(min_by, bucket - current->arData, &retval);
    ZEND_HASH_FOREACH_END();
    zval* min = zend_hash_minmax(min_by, cmp, 0);
    if (min)
    {
        zend_ulong offset = *(zend_ulong*)(min + 1);
        zval* ret = &(current->arData + offset)->val;
        RETVAL_ZVAL(ret, 1, 0);
    }
    else
    {
        RETVAL_NULL();
    }
    zend_hash_destroy(min_by);
    efree(min_by);
}

PHP_METHOD(Collection, minWith)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    FCI_G = &fci;
    FCC_G = &fcc;
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* min_with = zend_array_dup(current);
    ZEND_HASH_FOREACH_BUCKET(min_with, Bucket* bucket)
        NEW_PAIR_OBJ(obj);
        bucket_to_pair(obj, bucket);
        ZVAL_OBJ(&bucket->val, obj);
    ZEND_HASH_FOREACH_END();
    zval* min = PAIR_FETCH_SECOND(Z_OBJ_P(zend_hash_minmax(min_with, bucket_compare_userland, 0)));
    Z_TRY_ADDREF_P(min);
    zend_array_destroy(min_with);
    if (min)
    {
        RETURN_ZVAL(min, 0, 0);
    }
    RETVAL_NULL();
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
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            RETURN_FALSE;
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETURN_TRUE;
}

PHP_METHOD(Collection, onEach)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(Collection, packed)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    RETVAL_BOOL(HT_IS_PACKED(current));
}

PHP_METHOD(Collection, partition)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    NEW_PAIR_OBJ(pair);
    ARRAY_NEW_EX(first_arr, current);
    ARRAY_NEW_EX(second_arr, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zend_array* which = zend_is_true(&retval) ? first_arr : second_arr;
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key)
        {
            zend_hash_add(which, bucket->key, &bucket->val);
        }
        else if (HT_IS_PACKED(current))
        {
            zend_hash_next_index_insert(which, &bucket->val);
        }
        else
        {
            zend_hash_index_add(which, bucket->h, &bucket->val);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    zval first, second;
    ZVAL_ARR(&first, first_arr);
    ZVAL_ARR(&second, second_arr);
    PAIR_UPDATE_FIRST(pair, &first);
    PAIR_UPDATE_SECOND(pair, &second);
    RETVAL_OBJ(pair);
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
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    SEPARATE_CURRENT_COLLECTION(current);
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key)
        {
            zend_hash_update(current, bucket->key, &bucket->val);
        }
        else if (HT_IS_PACKED(elements_arr))
        {
            zend_hash_next_index_insert(current, &bucket->val);
        }
        else
        {
            zend_hash_index_update(current, bucket->h, &bucket->val);
        }
    ZEND_HASH_FOREACH_END();
}

PHP_METHOD(Collection, reduce)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    if (zend_hash_num_elements(current) == 0)
    {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    INIT_FCI(&fci, 3);
    Bucket* bucket = current->arData;
    Bucket* end = bucket + current->nNumUsed;
    for (; bucket < end; ++bucket)
    {
        if (Z_ISUNDEF(bucket->val))
        {
            continue;
        }
        ZVAL_COPY(&retval, &(bucket++)->val);
        break;
    }
    for (; bucket < end; ++bucket)
    {
        if (Z_ISUNDEF(bucket->val))
        {
            continue;
        }
        ZVAL_COPY_VALUE(&params[0], &retval);
        ZVAL_COPY_VALUE(&params[1], &bucket->val);
        if (bucket->key)
        {
            ZVAL_STR(&params[2], bucket->key);
        }
        else
        {
            ZVAL_LONG(&params[2], bucket->h);
        }
        zend_call_function(&fci, &fcc);
        zval_ptr_dtor(&params[0]);
    }
    RETVAL_ZVAL(&retval, 0, 0);
}

PHP_METHOD(Collection, reduceRight)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    if (zend_hash_num_elements(current) == 0)
    {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    INIT_FCI(&fci, 3);
    Bucket* start = current->arData;
    Bucket* bucket = start + current->nNumUsed - 1;
    for (; bucket >= start; --bucket)
    {
        if (Z_ISUNDEF(bucket->val))
        {
            continue;
        }
        ZVAL_COPY(&retval, &(bucket--)->val);
        break;
    }
    for (; bucket >= start; --bucket)
    {
        if (Z_ISUNDEF(bucket->val))
        {
            continue;
        }
        ZVAL_COPY_VALUE(&params[0], &retval);
        ZVAL_COPY_VALUE(&params[1], &bucket->val);
        if (bucket->key)
        {
            ZVAL_STR(&params[2], bucket->key);
        }
        else
        {
            ZVAL_LONG(&params[2], bucket->h);
        }
        zend_call_function(&fci, &fcc);
        zval_ptr_dtor(&params[0]);
    }
    RETVAL_ZVAL(&retval, 0, 0);
}

PHP_METHOD(Collection, remove)
{
    zval* key;
    zval* value = NULL;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zval* found;
    if (Z_TYPE_P(key) == IS_LONG)
    {
        if (value == NULL)
        {
            RETURN_BOOL(zend_hash_index_del(current, Z_LVAL_P(key)) == SUCCESS);
        }
        found = zend_hash_index_find(current, Z_LVAL_P(key));
        if (found == NULL || fast_equal_check_function(found, value) == 0)
        {
            RETURN_FALSE;
        }
        RETURN_BOOL(zend_hash_index_del(current, Z_LVAL_P(key)) == SUCCESS);
    }
    if (Z_TYPE_P(key) == IS_STRING)
    {
        if (value == NULL)
        {
            RETURN_BOOL(zend_hash_del(current, Z_STR_P(key)) == SUCCESS);
        }
        found = zend_hash_find(current, Z_STR_P(key));
        if (found == NULL || fast_equal_check_function(found, value) == 0)
        {
            RETURN_FALSE;
        }
        RETURN_BOOL(zend_hash_del(current, Z_STR_P(key)) == SUCCESS);
    }
    ERR_BAD_KEY_TYPE();
    RETVAL_FALSE;
}

PHP_METHOD(Collection, removeAll)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    SEPARATE_CURRENT_COLLECTION(current);
    if (EX_NUM_ARGS() == 0)
    {
        zend_hash_clean(current);
        return;
    }
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            zend_hash_del_bucket(current, bucket);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
}

PHP_METHOD(Collection, retainAll)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    SEPARATE_CURRENT_COLLECTION(current);
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval))
        {
            zend_hash_del_bucket(current, bucket);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
}

PHP_METHOD(Collection, reverse)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(reversed, current);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key)
        {
            zend_hash_add(reversed, bucket->key, &bucket->val);
        }
        else if (HT_IS_PACKED(current))
        {
            zend_hash_next_index_insert(reversed, &bucket->val);
        }
        else
        {
            zend_hash_index_add(reversed, bucket->h, &bucket->val);
        }
    ZEND_HASH_FOREACH_END();
    if (GC_REFCOUNT(current) > 1)
    {
        GC_DELREF(current);
    }
    else
    {
        zend_array_destroy(current);
    }
    COLLECTION_FETCH_CURRENT() = reversed;
}

PHP_METHOD(Collection, reversed)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(reversed, current);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key)
        {
            zend_hash_add(reversed, bucket->key, &bucket->val);
        }
        else if (HT_IS_PACKED(current))
        {
            zend_hash_next_index_insert(reversed, &bucket->val);
        }
        else
        {
            zend_hash_index_add(reversed, bucket->h, &bucket->val);
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(reversed);
}

PHP_METHOD(Collection, shuffle)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    uint32_t num_elements = zend_hash_num_elements(current);
    ARRAY_NEW(shuffled, num_elements);
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        zend_hash_next_index_insert(shuffled, val);
    ZEND_HASH_FOREACH_END();
    size_t offset = 0;
    Bucket* bucket = shuffled->arData;
    for (; offset < num_elements - 1; ++offset)
    {
        zend_long rand_idx = php_mt_rand_range(offset, num_elements - 1);
        zend_hash_bucket_renum_swap(&bucket[offset], &bucket[rand_idx]);
    }
    if (GC_REFCOUNT(current) > 1)
    {
        GC_DELREF(current);
    }
    else
    {
        zend_array_destroy(current);
    }
    COLLECTION_FETCH_CURRENT() = shuffled;
}

PHP_METHOD(Collection, shuffled)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    uint32_t num_elements = zend_hash_num_elements(current);
    ARRAY_NEW(shuffled, num_elements);
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        zend_hash_next_index_insert(shuffled, val);
    ZEND_HASH_FOREACH_END();
    size_t offset = 0;
    Bucket* bucket = shuffled->arData;
    for (; offset < num_elements - 1; ++offset)
    {
        zend_long rand_idx = php_mt_rand_range(offset, num_elements - 1);
        zend_hash_bucket_renum_swap(&bucket[offset], &bucket[rand_idx]);
    }
    RETVAL_NEW_COLLECTION(shuffled);
}

PHP_METHOD(Collection, set)
{
    zval* key;
    zval* value;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(key)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    if (Z_TYPE_P(key) == IS_STRING)
    {
        SEPARATE_CURRENT_COLLECTION(current);
        zend_hash_update(current, Z_STR_P(key), value);
    }
    else if (Z_TYPE_P(key) == IS_LONG)
    {
        SEPARATE_CURRENT_COLLECTION(current);
        zend_hash_index_update(current, Z_LVAL_P(key), value);
    }
    else
    {
        ERR_BAD_KEY_TYPE();
    }
}

PHP_METHOD(Collection, single)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zval single;
    ZVAL_UNDEF(&single);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            if (Z_TYPE(single) == IS_UNDEF)
            {
                ZVAL_COPY_VALUE(&single, &bucket->val);
            }
            else
            {
                RETURN_NULL();
            }
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    if (Z_TYPE(single) == IS_UNDEF)
    {
        RETURN_NULL();
    }
    RETVAL_ZVAL(&single, 1, 0);
}

PHP_METHOD(Collection, slice)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(sliced, current);
    ZEND_HASH_FOREACH_VAL(elements_arr, zval* val)
        if (Z_TYPE_P(val) == IS_LONG)
        {
            zval* found = zend_hash_index_find(current, Z_LVAL_P(val));
            if (found)
            {
                Z_TRY_ADDREF_P(found);
                zend_hash_next_index_insert(sliced, found);
            }
        }
        else if (Z_TYPE_P(val) == IS_STRING)
        {
            zval* found = zend_hash_find(current, Z_STR_P(val));
            if (found)
            {
                Z_TRY_ADDREF_P(found);
                zend_hash_add(sliced, Z_STR_P(val), found);
            }
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(sliced);
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
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    FCI_G = &fci;
    FCC_G = &fcc;
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* sorted_with = zend_array_dup(current);
    ZEND_HASH_FOREACH_BUCKET(sorted_with, Bucket* bucket)
        NEW_PAIR_OBJ(obj);
        bucket_to_pair(obj, bucket);
        ZVAL_OBJ(&bucket->val, obj);
    ZEND_HASH_FOREACH_END();
    zend_hash_sort(sorted_with, bucket_compare_userland, 1);
    ZEND_HASH_FOREACH_VAL(sorted_with, zval* val)
        zend_object* pair = Z_OBJ_P(val);
        ZVAL_COPY_VALUE(val, PAIR_FETCH_SECOND(pair));
        GC_DELREF(pair);
    ZEND_HASH_FOREACH_END();
    if (GC_REFCOUNT(current) > 1)
    {
        GC_DELREF(current);
    }
    else
    {
        zend_array_destroy(current);
    }
    COLLECTION_FETCH_CURRENT() = sorted_with;
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
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    FCI_G = &fci;
    FCC_G = &fcc;
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* sorted_with = zend_array_dup(current);
    ZEND_HASH_FOREACH_BUCKET(sorted_with, Bucket* bucket)
        NEW_PAIR_OBJ(obj);
        bucket_to_pair(obj, bucket);
        ZVAL_OBJ(&bucket->val, obj);
    ZEND_HASH_FOREACH_END();
    zend_hash_sort(sorted_with, bucket_compare_userland, 1);
    ZEND_HASH_FOREACH_VAL(sorted_with, zval* val)
        zend_object* pair = Z_OBJ_P(val);
        ZVAL_COPY_VALUE(val, PAIR_FETCH_SECOND(pair));
        GC_DELREF(pair);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(sorted_with);
}

PHP_METHOD(Collection, take)
{
    zend_long n;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(n)
    ZEND_PARSE_PARAMETERS_END();
    if (n < 0)
    {
        ERR_BAD_SIZE();
        return;
    }
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_long num_elements = zend_hash_num_elements(current);
    ARRAY_NEW(new_collection, n > num_elements ? num_elements : n);
    Bucket* bucket = current->arData;
    Bucket* end = bucket + current->nNumUsed;
    for (; n > 0 && bucket < end; ++bucket)
    {
        if (Z_ISUNDEF(bucket->val))
        {
            continue;
        }
        --n;
        Z_TRY_ADDREF(bucket->val);
        // Works for any zend_array, however, it doesn't make sense if you use any of these methods
        // on non-packed zend_arrays.
        if (bucket->key)
        {
            zend_hash_add_new(new_collection, bucket->key, &bucket->val);
        }
        else if (HT_IS_PACKED(current))
        {
            zend_hash_next_index_insert(new_collection, &bucket->val);
        }
        else
        {
            zend_hash_index_add_new(new_collection, bucket->h, &bucket->val);
        }
    }
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, takeLast)
{
    zend_long n;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(n)
    ZEND_PARSE_PARAMETERS_END();
    if (n < 0)
    {
        ERR_BAD_SIZE();
        return;
    }
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_long num_elements = zend_hash_num_elements(current);
    ARRAY_NEW(new_collection, n > num_elements ? num_elements : n);
    uint32_t idx = current->nNumUsed;
    zend_long num_taken = n;
    Bucket** taken = (Bucket**)malloc(num_taken * sizeof(Bucket*));
    // Note that the original element orders should be preserved as in kotlin.
    for (; num_taken > 0 && idx > 0; --idx)
    {
        Bucket* bucket = current->arData + idx - 1;
        if (Z_ISUNDEF(bucket->val))
        {
            continue;
        }
        taken[--num_taken] = bucket;
    }
    memset(&taken[0], 0, num_taken * sizeof(Bucket*));
    int i = 0;
    for (; i < n; ++i)
    {
        Bucket* bucket = taken[i];
        if (bucket == NULL)
        {
            continue;
        }
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key)
        {
            zend_hash_add_new(new_collection, bucket->key, &bucket->val);
        }
        else if (HT_IS_PACKED(current))
        {
            zend_hash_next_index_insert(new_collection, &bucket->val);
        }
        else
        {
            zend_hash_index_add_new(new_collection, bucket->h, &bucket->val);
        }
    }
    free(taken);
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, takeLastWhile)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    uint32_t num_elements = zend_hash_num_elements(current);
    Bucket** taken = (Bucket**)malloc(num_elements * sizeof(Bucket*));
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            taken[--num_elements] = bucket;
        }
        else
        {
            zval_ptr_dtor(&retval);
            break;
        }
    ZEND_HASH_FOREACH_END();
    memset(&taken[0], 0, num_elements * sizeof(Bucket*));
    int i = 0;
    for (; i < zend_hash_num_elements(current); ++i) {
        Bucket* bucket = taken[i];
        if (bucket == NULL)
        {
            continue;
        }
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key)
        {
            zend_hash_add_new(new_collection, bucket->key, &bucket->val);
        }
        else if (HT_IS_PACKED(current))
        {
            zend_hash_next_index_insert(new_collection, &bucket->val);
        }
        else
        {
            zend_hash_index_add_new(new_collection, bucket->h, &bucket->val);
        }
    }
    free(taken);
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, takeWhile)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval))
        {
            zval_ptr_dtor(&retval);
            if (bucket->key)
            {
                zend_hash_add_new(new_collection, bucket->key, &bucket->val);
            }
            else if (HT_IS_PACKED(current))
            {
                zend_hash_next_index_insert(new_collection, &bucket->val);
            }
            else
            {
                zend_hash_index_add_new(new_collection, bucket->h, &bucket->val);
            }
        }
        else
        {
            zval_ptr_dtor(&retval);
            break;
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, toArray)
{
    zend_array* retval = COLLECTION_FETCH_CURRENT();
    GC_ADDREF(retval);
    RETVAL_ARR(retval);
}

PHP_METHOD(Collection, toCollection)
{
    zval* dest;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(dest, collections_collection_ce)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* dest_arr = COLLECTION_FETCH(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        if (bucket->key)
        {
            zend_hash_add(dest_arr, bucket->key, &bucket->val);
        }
        else
        {
            zend_hash_next_index_insert(dest_arr, &bucket->val);
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, toPairs)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        NEW_PAIR_OBJ(obj);
        bucket_to_pair(obj, bucket);
        zval pair;
        ZVAL_OBJ(&pair, obj);
        zend_hash_next_index_insert(new_collection, &pair);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, union)
{
    
}

PHP_METHOD(Collection, values)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        zend_hash_next_index_insert(new_collection, val);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Pair, __construct)
{
    zval* first;
    zval* second;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(first)
        Z_PARAM_ZVAL(second)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* properties = Z_OBJ_P(getThis())->properties = (zend_array*)emalloc(sizeof(zend_array));
    zend_hash_init(properties, 2, NULL, ZVAL_PTR_DTOR, 0);
    PAIR_UPDATE_FIRST(Z_OBJ_P(getThis()), first);
    PAIR_UPDATE_SECOND(Z_OBJ_P(getThis()), second);
}
