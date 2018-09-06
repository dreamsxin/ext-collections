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

#if PHP_VERSION_ID < 70300
#define GC_ADDREF(p)                ++GC_REFCOUNT(p)
#define GC_DELREF(p)                --GC_REFCOUNT(p)
#endif

#define FCI_G                       COLLECTIONS_G(fci)
#define FCC_G                       COLLECTIONS_G(fcc)

#define COLLECTION_FETCH(val)       (Z_OBJ_P(val)->properties)
#define COLLECTION_FETCH_CURRENT()  COLLECTION_FETCH(getThis())
#define PAIR_FIRST(obj)             OBJ_PROP_NUM(obj, 0)
#define PAIR_SECOND(obj)            OBJ_PROP_NUM(obj, 1)

#define IS_COLLECTION_P(zval)                                              \
    Z_TYPE_P(zval) == IS_OBJECT && Z_OBJCE_P(zval) == collections_collection_ce
#define IS_PAIR(zval)                                                      \
    Z_TYPE(zval) == IS_OBJECT && Z_OBJCE(zval) == collections_pair_ce

#define SEPARATE_COLLECTION(ht, obj)                                       \
    if (GC_REFCOUNT(ht) > 1) {                                             \
        GC_DELREF(ht);                                                     \
        ht = Z_OBJ_P(obj)->properties = zend_array_dup(ht);                \
    }
#define SEPARATE_CURRENT_COLLECTION(ht)                                    \
    SEPARATE_COLLECTION(ht, getThis())

#define INIT_FCI(fci, num_args)                                            \
    zval params[num_args], retval;                                         \
    (fci)->size = sizeof(zend_fcall_info);                                 \
    (fci)->param_count = num_args;                                         \
    (fci)->retval = &retval;                                               \
    (fci)->params = params

#define CALLBACK_KEYVAL_INVOKE(params, bucket)                             \
    ZVAL_COPY_VALUE(&params[0], &bucket->val);                             \
    if ((bucket)->key) {                                                   \
        ZVAL_STR(&params[1], (bucket)->key);                               \
    } else {                                                               \
        ZVAL_LONG(&params[1], (bucket)->h);                                \
    }                                                                      \
    zend_call_function(&fci, &fcc)

#define PHP_COLLECTIONS_ERROR(type, msg)                                   \
    php_error_docref(NULL, type, msg)
#define ERR_BAD_ARGUMENT_TYPE()                                            \
    PHP_COLLECTIONS_ERROR(E_WARNING, "Bad argument type")
#define ERR_BAD_KEY_TYPE()                                                 \
    PHP_COLLECTIONS_ERROR(E_WARNING, "Key must be integer or string")
#define ERR_BAD_CALLBACK_RETVAL()                                          \
    PHP_COLLECTIONS_ERROR(E_WARNING, "Bad callback return value")
#define ERR_BAD_SIZE()                                                     \
    PHP_COLLECTIONS_ERROR(E_WARNING, "Size must be non-negative")
#define ERR_BAD_INDEX()                                                    \
    PHP_COLLECTIONS_ERROR(E_WARNING, "Index must be non-negative")
#define ERR_NOT_NUMERIC()                                                  \
    PHP_COLLECTIONS_ERROR(E_WARNING, "Elements should be int or double")
#define ERR_BAD_GROUP()                                                    \
    PHP_COLLECTIONS_ERROR(E_WARNING, "Group value must be array")
#define ERR_SILENCED()

#define ELEMENTS_VALIDATE(elements, err, err_then)                         \
    zend_array* elements##_arr;                                            \
    if (IS_COLLECTION_P(elements)) {                                       \
        (elements##_arr) = COLLECTION_FETCH(elements);                     \
    } else if (UNEXPECTED(Z_TYPE_P(elements) == IS_ARRAY)) {               \
        (elements##_arr) = Z_ARRVAL_P(elements);                           \
    } else {                                                               \
        err();                                                             \
        err_then;                                                          \
    }

#define ARRAY_NEW(name, size)                                              \
    zend_array* (name) = (zend_array*)emalloc(sizeof(zend_array));         \
    zend_hash_init(name, size, NULL, ZVAL_PTR_DTOR, 0)
#define ARRAY_NEW_EX(name, other)                                          \
    ARRAY_NEW(name, zend_hash_num_elements(other))
#define ARRAY_CLONE(dest, src)                                             \
    zend_array* (dest) = zend_array_dup(src);

#define RETVAL_NEW_COLLECTION(collection)                                  \
    {                                                                      \
        zend_object* obj = create_collection_obj();                        \
        if (GC_REFCOUNT(collection) > 1)                                   \
            GC_ADDREF(collection);                                         \
        obj->properties = collection;                                      \
        RETVAL_OBJ(obj);                                                   \
    }

#define RETURN_NEW_COLLECTION(collection)                                  \
    {                                                                      \
        RETVAL_NEW_COLLECTION(collection);                                 \
        return;                                                            \
    }

typedef int (*equal_check_func_t)(zval*, zval*);

/// Unused global variable.
zval rv;

static zend_always_inline void pair_update_first(zend_object* obj, zval* value)
{
    zval_ptr_dtor(PAIR_FIRST(obj));
    ZVAL_COPY_VALUE(PAIR_FIRST(obj), value);
}

static zend_always_inline void pair_update_second(zend_object* obj, zval* value)
{
    zval_ptr_dtor(PAIR_SECOND(obj));
    ZVAL_COPY_VALUE(PAIR_SECOND(obj), value);
}

static zend_always_inline zend_object* create_object(zend_class_entry* ce,
    zend_object_handlers* handlers)
{
    zend_object* obj = (zend_object*)ecalloc(1, sizeof(zend_object) +
        zend_object_properties_size(ce));
    zend_object_std_init(obj, ce);
    object_properties_init(obj, ce);
    obj->handlers = handlers;
    return obj;
}

static zend_always_inline zend_object* create_collection_obj()
{
    return create_object(collections_collection_ce, &collection_handlers);
}

static zend_always_inline zend_object* create_pair_obj()
{
    return create_object(collections_pair_ce, &std_object_handlers);
}

static zend_always_inline zend_array* array_group_fetch(zend_array* ht, zval* key)
{
    zend_array* group = NULL;
    if (Z_TYPE_P(key) == IS_LONG) {
        zval* group_val = zend_hash_index_find(ht, Z_LVAL_P(key));
        if (UNEXPECTED(group_val == NULL)) {
            zval tmp_val;
            group = (zend_array*)emalloc(sizeof(zend_array));
            zend_hash_init(group, 8, NULL, ZVAL_PTR_DTOR, 0);
            ZVAL_ARR(&tmp_val, group);
            zend_hash_index_add(ht, Z_LVAL_P(key), &tmp_val);
        } else if (EXPECTED(Z_TYPE_P(group_val) == IS_ARRAY)) {
            SEPARATE_ARRAY(group_val);
            group = Z_ARR_P(group_val);
        } else {
            ERR_BAD_GROUP();
        }
    } else if (Z_TYPE_P(key) == IS_STRING) {
        zval* group_val = zend_hash_find(ht, Z_STR_P(key));
        if (UNEXPECTED(group_val == NULL)) {
            zval tmp_val;
            group = (zend_array*)emalloc(sizeof(zend_array));
            zend_hash_init(group, 8, NULL, ZVAL_PTR_DTOR, 0);
            ZVAL_ARR(&tmp_val, group);
            zend_hash_add(ht, Z_STR_P(key), &tmp_val);
        } else if (EXPECTED(Z_TYPE_P(group_val) == IS_ARRAY)) {
            SEPARATE_ARRAY(group_val);
            group = Z_ARR_P(group_val);
        } else {
            ERR_BAD_GROUP();
        }
    } else {
        ERR_BAD_KEY_TYPE();
    }
    return group;
}

static zend_always_inline void bucket_to_pair(zend_object* pair, Bucket* bucket)
{
    zval key;
    if (bucket->key) {
        GC_ADDREF(bucket->key);
        ZVAL_STR(&key, bucket->key);
    } else {
        ZVAL_LONG(&key, bucket->h);
    }
    pair_update_first(pair, &key);
    pair_update_second(pair, &bucket->val);
}

static int bucket_compare_numeric(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    return numeric_compare_function(&b1->val, &b2->val);
}

static int bucket_reverse_compare_numeric(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    return numeric_compare_function(&b2->val, &b1->val);
}

static int bucket_compare_string_ci(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    return string_case_compare_function(&b1->val, &b2->val);
}

static int bucket_reverse_compare_string_ci(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    return string_case_compare_function(&b2->val, &b1->val);
}

static int bucket_compare_string_cs(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    return string_compare_function(&b1->val, &b2->val);
}

static int bucket_reverse_compare_string_cs(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    return string_compare_function(&b2->val, &b1->val);
}

static int bucket_compare_natural_ci(const void* op1, const void* op2)
{
    zend_string* s1 = Z_STR(((Bucket*)op1)->val);
    zend_string* s2 = Z_STR(((Bucket*)op2)->val);
    return strnatcmp_ex(ZSTR_VAL(s1), ZSTR_LEN(s1), ZSTR_VAL(s2), ZSTR_LEN(s2), 1);
}

static int bucket_reverse_compare_natural_ci(const void* op1, const void* op2)
{
    zend_string* s1 = Z_STR(((Bucket*)op1)->val);
    zend_string* s2 = Z_STR(((Bucket*)op2)->val);
    return strnatcmp_ex(ZSTR_VAL(s2), ZSTR_LEN(s2), ZSTR_VAL(s1), ZSTR_LEN(s1), 1);
}

static int bucket_compare_natural_cs(const void* op1, const void* op2)
{
    zend_string* s1 = Z_STR(((Bucket*)op1)->val);
    zend_string* s2 = Z_STR(((Bucket*)op2)->val);
    return strnatcmp_ex(ZSTR_VAL(s1), ZSTR_LEN(s1), ZSTR_VAL(s2), ZSTR_LEN(s2), 0);
}

static int bucket_reverse_compare_natural_cs(const void* op1, const void* op2)
{
    zend_string* s1 = Z_STR(((Bucket*)op1)->val);
    zend_string* s2 = Z_STR(((Bucket*)op2)->val);
    return strnatcmp_ex(ZSTR_VAL(s2), ZSTR_LEN(s2), ZSTR_VAL(s1), ZSTR_LEN(s1), 0);
}

static int bucket_compare_regular(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    zval result;
    if (compare_function(&result, &b1->val, &b2->val) == FAILURE) {
        return 0;
    }
    return ZEND_NORMALIZE_BOOL(Z_LVAL(result));
}

static int bucket_reverse_compare_regular(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    zval result;
    if (compare_function(&result, &b2->val, &b1->val) == FAILURE) {
        return 0;
    }
    return ZEND_NORMALIZE_BOOL(Z_LVAL(result));
}

static int bucket_compare_by(const void* op1, const void* op2)
{
    Bucket* b1 = (Bucket*)op1;
    Bucket* b2 = (Bucket*)op2;
    zval* ref = COLLECTIONS_G(ref);
    compare_func_t cmp = COLLECTIONS_G(cmp);
    return cmp(&ref[b1->h], &ref[b2->h]);
}

static int bucket_compare_with_idx(const void* op1, const void* op2)
{
    zend_long h1 = ((Bucket*)op1)->h;
    zend_long h2 = ((Bucket*)op2)->h;
    compare_func_t cmp = COLLECTIONS_G(cmp);
    int result = cmp(op1, op2);
    return result ? result : ZEND_NORMALIZE_BOOL(h1 - h2);
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
    if (Z_TYPE_P(val) == IS_LONG) {
        return fast_equal_check_long;
    }
    if (Z_TYPE_P(val) == IS_STRING) {
        return fast_equal_check_string;
    } 
    return fast_equal_check_function;
}

static zend_always_inline compare_func_t compare_func_init(
    zval* val, zend_bool reverse, zend_long flags)
{
    zend_bool case_insensitive = flags & PHP_COLLECTIONS_FOLD_CASE;
    if (Z_TYPE_P(val) == IS_LONG || Z_TYPE_P(val) == IS_DOUBLE) {
        return reverse ? bucket_reverse_compare_numeric : bucket_compare_numeric;
    }
    if (Z_TYPE_P(val) == IS_STRING) {
        if ((flags & ~PHP_COLLECTIONS_FOLD_CASE) == PHP_COLLECTIONS_COMPARE_NATURAL) {
            if (case_insensitive) {
                return reverse ? bucket_reverse_compare_natural_ci : bucket_compare_natural_ci;
            }
            return reverse ? bucket_reverse_compare_natural_cs : bucket_compare_natural_cs;
        }
        if (case_insensitive) {
            return reverse ? bucket_reverse_compare_string_ci : bucket_compare_string_ci;
        }
        return reverse ? bucket_reverse_compare_string_cs : bucket_compare_string_cs;
    }
    return reverse ? bucket_reverse_compare_regular : bucket_compare_regular;
}

static zend_always_inline void zend_hash_sort_by(zend_array* ht)
{
    uint32_t i;
    if (HT_IS_WITHOUT_HOLES(ht)) {
        i = ht->nNumUsed;
    } else {
        uint32_t j;
        for (j = 0, i = 0; j < ht->nNumUsed; j++) {
            Bucket *p = ht->arData + j;
            if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) {
                continue;
            }
            if (i != j) {
                ht->arData[i] = *p;
            }
            i++;
        }
    }
    uint32_t num_elements = zend_hash_num_elements(ht);
    if (num_elements > 1) {
        zend_sort(ht->arData, i, sizeof(Bucket), bucket_compare_by,
            (swap_func_t)zend_hash_bucket_packed_swap);
        ht->nNumUsed = i;
    }
    uint32_t idx = 0;
    ZEND_HASH_FOREACH_BUCKET(ht, Bucket* bucket)
        bucket->h = idx++;
    ZEND_HASH_FOREACH_END();
    if (!HT_IS_PACKED(ht)) {
        zend_hash_to_packed(ht);
    }
}

static zend_always_inline void array_distinct(zend_array* ht, Bucket* ref, compare_func_t cmp,
    equal_check_func_t eql)
{
    uint32_t num_elements = zend_hash_num_elements(ht);
    zend_bool packed = HT_IS_PACKED(ht);
    uint32_t idx = 0;
    zend_sort(ref, num_elements, sizeof(Bucket), packed ? bucket_compare_with_idx : cmp,
        (swap_func_t)zend_hash_bucket_packed_swap);
    Bucket* first = &ref[0];
    if (packed) {
        for (idx = 1; idx < num_elements; ++idx) {
            Bucket* bucket = &ref[idx];
            if (eql(&bucket->val, &first->val)) {
                Bucket* duplicate = ht->arData + bucket->h;
                zval_ptr_dtor(&duplicate->val);
                ZVAL_UNDEF(&duplicate->val);
                --ht->nNumOfElements;
            } else {
                first = bucket;
            }
        }
        // Renumber the integer keys and return a new Collection with packed zend_array.
        Bucket* bucket = ht->arData;
        Bucket* last = NULL;
        for (idx = 0; bucket < ht->arData + ht->nNumUsed; ++bucket, ++idx) {
            bucket->h = idx;
            if (Z_ISUNDEF(bucket->val)) {
                if (UNEXPECTED(last == NULL)) {
                    last = bucket;
                }
            } else if (EXPECTED(last)) {
                ZVAL_COPY_VALUE(&(last++)->val, &bucket->val);
                ZVAL_UNDEF(&bucket->val);
            }
        }
        ht->nNumUsed = ht->nNumOfElements;
        zend_hash_to_packed(ht);
    } else {
        for (idx = 1; idx < num_elements; ++idx) {
            Bucket* bucket = &ref[idx];
            if (eql(&bucket->val, &first->val)) {
                zend_hash_del_bucket(ht, ht->arData + bucket->h);
            } else {
                first = bucket;
            }
        }
    }
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
    if (check_empty) {
        zval result;
        return zend_is_true(collection_offset_get(object, offset, 0, &result));
    }
    if (Z_TYPE_P(offset) == IS_LONG) {
        return zend_hash_index_exists(current, Z_LVAL_P(offset));
    }
    if (Z_TYPE_P(offset) == IS_STRING) {
        return zend_hash_exists(current, Z_STR_P(offset));
    }
    return 0;
}

int collection_property_exists(zval* object, zval* member, int has_set_exists,
    void** unused)
{
    zend_array* current = COLLECTION_FETCH(object);
    zval* found = NULL;
    if (EXPECTED(Z_TYPE_P(member) == IS_STRING)) {
        found = zend_hash_find(current, Z_STR_P(member));
    } else if (EXPECTED(Z_TYPE_P(member) == IS_LONG)) {
        found = zend_hash_index_find(current, Z_LVAL_P(member));
    }
    if (found == NULL) {
        return 0;
    }
    if (has_set_exists == 0) {
        // whether property exists and is not NULL
        return Z_TYPE_P(found) != IS_NULL;
    } else if (has_set_exists == 1) {
        // whether property exists and is true
        return zend_is_true(found);
    }
    // whether property exists
    return 1;
}

void collection_offset_set(zval* object, zval* offset, zval* value)
{
    zend_array* current = COLLECTION_FETCH(object);
    SEPARATE_COLLECTION(current, object);
    if (Z_TYPE_P(offset) == IS_LONG) {
        zend_hash_index_update(current, Z_LVAL_P(offset), value);
    } else if (Z_TYPE_P(offset) == IS_STRING) {
        zend_hash_update(current, Z_STR_P(offset), value);
    } else {
        ERR_BAD_KEY_TYPE();
        return;
    }
    Z_TRY_ADDREF_P(value);
}

void collection_property_set(zval* object, zval* member, zval* value, void** unused)
{
    collection_offset_set(object, member, value);
}

zval* collection_offset_get(zval* object, zval* offset, int type, zval* retval)
{
    // Note that we don't handle type. So don't do any fancy things with Collection
    // such as fetching a reference of a value, etc.
    zend_array* current = COLLECTION_FETCH(object);
    zval* found = NULL;
    if (Z_TYPE_P(offset) == IS_LONG) {
        found = zend_hash_index_find(current, Z_LVAL_P(offset));
    } else if (Z_TYPE_P(offset) == IS_STRING) {
        found = zend_hash_find(current, Z_STR_P(offset));
    } if (found) {
        ZVAL_COPY_VALUE(retval, found);
    } else {
        retval = &EG(uninitialized_zval);
    }
    return retval;
}

zval* collection_property_get(zval* object, zval* member, int type, void** unused,
    zval* retval)
{
    return collection_offset_get(object, member, type, retval);
}

void collection_offset_unset(zval* object, zval* offset)
{
    zend_array* current = COLLECTION_FETCH(object);
    SEPARATE_COLLECTION(current, object);
    if (Z_TYPE_P(offset) == IS_LONG) {
        zend_hash_index_del(current, Z_LVAL_P(offset));
    } else if (Z_TYPE_P(offset) == IS_STRING) {
        zend_hash_del(current, Z_STR_P(offset));
    }
}

void collection_property_unset(zval* object, zval* member, void** unused)
{
    collection_offset_unset(object, member);
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
    zend_bool packed = HT_IS_PACKED(current) && HT_IS_PACKED(elements_arr);
    SEPARATE_CURRENT_COLLECTION(current);
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        zval* result;
        if (bucket->key) {
            result = zend_hash_add(current, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(current, &bucket->val);
        } else {
            result = zend_hash_index_add(current, bucket->h, &bucket->val);
        }
        if (result) {
            Z_TRY_ADDREF(bucket->val);
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
        if (!zend_is_true(&retval)) {
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
        if (zend_is_true(&retval)) {
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
        if (EXPECTED(IS_PAIR(retval))) {
            zval* key = PAIR_FIRST(Z_OBJ(retval));
            zval* value = PAIR_SECOND(Z_OBJ(retval));
            if (Z_TYPE_P(key) == IS_LONG) {
                Z_TRY_ADDREF_P(value);
                zend_hash_index_add(new_collection, Z_LVAL_P(key), value);
            } else if (Z_TYPE_P(key) == IS_STRING) {
                Z_TRY_ADDREF_P(value);
                zend_hash_add(new_collection, Z_STR_P(key), value);
            } else {
                ERR_BAD_KEY_TYPE();
            }
        } else {
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
        if (EXPECTED(IS_PAIR(retval))) {
            zval* key = PAIR_FIRST(Z_OBJ(retval));
            zval* value = PAIR_SECOND(Z_OBJ(retval));
            if (Z_TYPE_P(key) == IS_LONG) {
                value = zend_hash_index_add(dest_arr, Z_LVAL_P(key), value);
            } else if (Z_TYPE_P(key) == IS_STRING) {
                value = zend_hash_add(dest_arr, Z_STR_P(key), value);
            } else {
                ERR_BAD_KEY_TYPE();
                value = NULL;
            }
            if (value) {
                Z_TRY_ADDREF_P(value);
            }
        } else {
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
        if (Z_TYPE(retval) == IS_LONG) {
            Z_TRY_ADDREF(bucket->val);
            zend_hash_index_add(new_collection, Z_LVAL(retval), &bucket->val);
        } else if (Z_TYPE(retval) == IS_STRING) {
            Z_TRY_ADDREF(bucket->val);
            zend_hash_add(new_collection, Z_STR(retval), &bucket->val);
        } else {
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
        zval* result = NULL;
        if (Z_TYPE(retval) == IS_LONG) {
            result = zend_hash_index_add(dest_arr, Z_LVAL(retval), &bucket->val);
        } else if (Z_TYPE(retval) == IS_STRING) {
            result = zend_hash_add(dest_arr, Z_STR(retval), &bucket->val);
        } else {
            ERR_BAD_CALLBACK_RETVAL();
        }
        if (result) {
            Z_TRY_ADDREF(bucket->val);
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
        if (Z_TYPE_P(val) == IS_LONG) {
            sum += Z_LVAL_P(val);
        } else if (Z_TYPE_P(val) == IS_DOUBLE) {
            sum += Z_DVAL_P(val);
        } else {
            ERR_NOT_NUMERIC();
            RETURN_FALSE;
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
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        if (bucket->key) {
            zval* result = zend_hash_find(current, bucket->key);
            if (!result || !fast_equal_check_function(&bucket->val, result)) {
                RETURN_FALSE;
            }
        } else {
            zval* result = zend_hash_index_find(current, bucket->h);
            if (!result || !fast_equal_check_function(&bucket->val, result)) {
                RETURN_FALSE;
            }
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_TRUE;
}

PHP_METHOD(Collection, containsAllKeys)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        if (bucket->key) {
            if (!zend_hash_exists(current, bucket->key)) {
                RETURN_FALSE;
            }
        } else {
            if (!zend_hash_index_exists(current, bucket->h)) {
                RETURN_FALSE;
            }
        }
    ZEND_HASH_FOREACH_END();
    RETURN_TRUE;
}

PHP_METHOD(Collection, containsAllValues)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    compare_func_t cmp = NULL;
    equal_check_func_t eql = NULL;
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        cmp = compare_func_init(val, 0, 0);
        eql = equal_check_func_init(val);
        break;
    ZEND_HASH_FOREACH_END();
    ARRAY_CLONE(sorted_current, current);
    zend_hash_sort(sorted_current, cmp, 1);
    ARRAY_CLONE(sorted_other, elements_arr);
    zend_hash_sort(sorted_other, cmp, 1);
    Bucket* this = NULL;
    Bucket* other = sorted_other->arData;
    Bucket* end_other = other + zend_hash_num_elements(sorted_other);
    zend_bool result = 1;
    ZEND_HASH_FOREACH_BUCKET(sorted_current, Bucket* bucket)
        if (EXPECTED(this) && eql(&bucket->val, &this->val)) {
            continue;
        }
        this = bucket;
        if (cmp(bucket, other)) {
            result = 0;
            break;
        }
        do {
            if (UNEXPECTED(++other == end_other)) {
                goto end;
            }
        } while (eql(&(other - 1)->val, &other->val));
    ZEND_HASH_FOREACH_END();
    result = 0;
    do {
        if (UNEXPECTED(++other == end_other)) {
            result = 1;
            break;
        }
    } while (eql(&(other - 1)->val, &other->val));
end:
    zend_array_destroy(sorted_current);
    zend_array_destroy(sorted_other);
    RETVAL_BOOL(result);
}

PHP_METHOD(Collection, containsKey)
{
    zval* key;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(key)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    if (Z_TYPE_P(key) == IS_LONG) {
        RETURN_BOOL(zend_hash_index_exists(current, Z_LVAL_P(key)));
    }
    if (Z_TYPE_P(key) == IS_STRING) {
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
        if (eql(element, val)) {
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
    uint32_t num_elements = zend_hash_num_elements(current);
    ARRAY_NEW(new_collection, new_size < num_elements ? new_size : num_elements);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_add_new(new_collection, bucket->key, &bucket->val);
        } else {
            zend_hash_index_add_new(new_collection, bucket->h, &bucket->val);
        }
        if (--new_size == 0) {
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
    if (from_idx < 0) {
        ERR_BAD_INDEX();
        RETURN_NULL();
    }
    if (num_elements < 0) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW(new_collection, num_elements);
    zend_bool packed = HT_IS_PACKED(current);
    Bucket* bucket = current->arData;
    Bucket* end = bucket + current->nNumUsed;
    for (bucket += from_idx; num_elements > 0 && bucket < end; ++bucket) {
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        --num_elements;
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_add(new_collection, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(new_collection, &bucket->val);
        } else {
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
    zend_array* current = COLLECTION_FETCH_CURRENT();
    compare_func_t cmp = NULL;
    equal_check_func_t eql = NULL;
    Bucket* ref = (Bucket*)malloc(zend_hash_num_elements(current) * sizeof(Bucket));
    ARRAY_CLONE(distinct, current);
    uint32_t idx = 0;
    ZEND_HASH_FOREACH_BUCKET(distinct, Bucket* bucket)
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&bucket->val, 0, 0);
            eql = equal_check_func_init(&bucket->val);
        }
        Bucket* dest = &ref[idx++];
        dest->key = NULL;
        dest->h = bucket - distinct->arData;
        memcpy(&dest->val, &bucket->val, sizeof(zval));
    ZEND_HASH_FOREACH_END();
    COLLECTIONS_G(cmp) = cmp;
    array_distinct(distinct, ref, cmp, eql);
    free(ref);
    RETVAL_NEW_COLLECTION(distinct);
}

PHP_METHOD(Collection, distinctBy)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    compare_func_t cmp = NULL;
    equal_check_func_t eql = NULL;
    uint32_t num_elements = zend_hash_num_elements(current);
    Bucket* ref = (Bucket*)malloc(num_elements * sizeof(Bucket));
    ARRAY_CLONE(distinct, current);
    uint32_t idx = 0;
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(distinct, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&retval, 0, 0);
            eql = equal_check_func_init(&retval);
        }
        Bucket* dest = &ref[idx++];
        dest->key = NULL;
        dest->h = bucket - distinct->arData;
        memcpy(&dest->val, &retval, sizeof(zval));
    ZEND_HASH_FOREACH_END();
    COLLECTIONS_G(cmp) = cmp;
    array_distinct(distinct, ref, cmp, eql);
    for (idx = 0; idx < num_elements; ++idx) {
        zval_ptr_dtor(&ref[idx].val);
    }
    free(ref);
    RETVAL_NEW_COLLECTION(distinct);
}

PHP_METHOD(Collection, drop)
{
    zend_long n;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(n)
    ZEND_PARSE_PARAMETERS_END();
    if (n < 0) {
        ERR_BAD_SIZE();
        return;
    }
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_CLONE(new_collection, current);
    Bucket* bucket = new_collection->arData;
    Bucket* end = bucket + new_collection->nNumUsed;
    for (; n > 0 && bucket < end; ++bucket) {
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        --n;
        zval_ptr_dtor(&bucket->val);
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
        return;
    }
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_CLONE(new_collection, current);
    unsigned idx = new_collection->nNumUsed;
    for (; n > 0 && idx > 0; --idx) {
        Bucket* bucket = new_collection->arData + idx - 1;
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        --n;
        zval_ptr_dtor(&bucket->val);
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
        if (zend_is_true(&retval)) {
            zval_ptr_dtor(&retval);
            zend_hash_del_bucket(new_collection, bucket);
        } else {
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
        if (zend_is_true(&retval)) {
            zval_ptr_dtor(&retval);
            zend_hash_del_bucket(new_collection, bucket);
        } else {
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
    Bucket* bucket = current->arData + from_idx;
    Bucket* end = bucket + current->nNumUsed;
    for (; num_elements > 0 && bucket < end; ++bucket, --num_elements) {
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        Z_TRY_ADDREF_P(element);
        if (bucket->key) {
            zend_hash_update(current, bucket->key, element);
        } else {
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
    ARRAY_NEW(new_collection, 8);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval)) {
            Z_TRY_ADDREF(bucket->val);
            if (bucket->key) {
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            } else if (packed) {
                zend_hash_next_index_insert(new_collection, &bucket->val);
            } else {
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
    ARRAY_NEW(new_collection, 8);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval)) {
            Z_TRY_ADDREF(bucket->val);
            if (bucket->key) {
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            } else if (packed) {
                zend_hash_next_index_insert(new_collection, &bucket->val);
            } else {
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
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval)) {
            zval* result = NULL;
            if (bucket->key) {
                result = zend_hash_add(dest_arr, bucket->key, &bucket->val);
            } else if (packed) {
                zend_hash_next_index_insert(dest_arr, &bucket->val);
            } else {
                result = zend_hash_index_add(dest_arr, bucket->h, &bucket->val);
            }
            if (result) {
                Z_TRY_ADDREF(bucket->val);
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
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval)) {
            zval* result;
            if (bucket->key) {
                result = zend_hash_add(dest_arr, bucket->key, &bucket->val);
            } else if (packed) {
                zend_hash_next_index_insert(dest_arr, &bucket->val);
            } else {
                result = zend_hash_index_add(dest_arr, bucket->h, &bucket->val);
            }
            if (result) {
                Z_TRY_ADDREF(bucket->val);
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
    if (zend_hash_num_elements(current) == 0) {
        RETURN_NULL();
    }
    if (EX_NUM_ARGS() == 0) {
        uint32_t pos = 0;
        while (pos < current->nNumUsed && Z_ISUNDEF(current->arData[pos].val)) {
            ++pos;
        }
        RETURN_ZVAL(&current->arData[pos].val, 1, 0);
    }
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval)) {
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
            if (bucket->key) {
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            } else {
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
            if (bucket->key) {
                zend_hash_add(dest_arr, bucket->key, &bucket->val);
            } else {
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
        ELEMENTS_VALIDATE(val, ERR_SILENCED, {
            Z_TRY_ADDREF_P(val);
            if (bucket->key) {
                zend_hash_add(new_collection, bucket->key, val);
            } else {
                zend_hash_next_index_insert(new_collection, val);
            }
            continue;
        });
        ZEND_HASH_FOREACH_BUCKET(val_arr, Bucket* bucket)
            Z_TRY_ADDREF(bucket->val);
            if (bucket->key) {
                zend_hash_add(new_collection, bucket->key, &bucket->val);
            } else {
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
        if (bucket->key) {
            ZVAL_STR(&params[2], bucket->key);
        } else {
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
        if (bucket->key) {
            ZVAL_STR(&params[2], bucket->key);
        } else {
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
    if (Z_TYPE_P(key) == IS_STRING) {
        found = zend_hash_find(current, Z_STR_P(key));
    } else if (Z_TYPE_P(key) == IS_LONG) {
        found = zend_hash_index_find(current, Z_LVAL_P(key));
    } else {
        ERR_BAD_KEY_TYPE();
        RETURN_NULL();
    }
    if (found) {
        RETURN_ZVAL(found, 1, 0);
    }
    if (EX_NUM_ARGS() < 2) {
        RETURN_NULL();
    }
    INIT_FCI(&fci, 1);
    ZVAL_COPY_VALUE(&params[0], key);
    zend_call_function(&fci, &fcc);
    RETVAL_ZVAL(&retval, 0, 0);
}

PHP_METHOD(Collection, groupBy)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW(new_collection, 8);
    zend_bool packed = HT_IS_PACKED(current);
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zval* key;
        zval* value;
        if (EXPECTED(IS_PAIR(retval))) {
            key = PAIR_FIRST(Z_OBJ(retval));
            value = PAIR_SECOND(Z_OBJ(retval));
        } else {
            key = &retval;
            value = &bucket->val;
        }
        zend_array* group = array_group_fetch(new_collection, key);
        if (UNEXPECTED(group == NULL)) {
            continue;
        }
        Z_TRY_ADDREF_P(value);
        if (bucket->key) {
            zend_hash_add(group, bucket->key, value);
        } else if (packed) {
            zend_hash_next_index_insert(group, value);
        } else {
            zend_hash_index_add(group, bucket->h, value);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, groupByTo)
{
    zval* dest;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_OBJECT_OF_CLASS(dest, collections_collection_ce)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_array* dest_arr = COLLECTION_FETCH(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    zend_bool packed = HT_IS_PACKED(current);
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zval* key;
        zval* value;
        if (EXPECTED(IS_PAIR(retval))) {
            key = PAIR_FIRST(Z_OBJ(retval));
            value = PAIR_SECOND(Z_OBJ(retval));
        } else {
            key = &retval;
            value = &bucket->val;
        }
        zend_array* group = array_group_fetch(dest_arr, key);
        if (UNEXPECTED(group == NULL)) {
            continue;
        }
        if (bucket->key) {
            value = zend_hash_add(group, bucket->key, value);
        } else if (packed) {
            zend_hash_next_index_insert(group, value);
        } else {
            value = zend_hash_index_add(group, bucket->h, value);
        }
        if (value) {
            Z_TRY_ADDREF_P(value);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(dest_arr);
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
        if (eql(element, &bucket->val)) {
            if (bucket->key) {
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
        if (zend_is_true(&retval)) {
            zval_ptr_dtor(&retval);
            if (bucket->key) {
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
        if (zend_is_true(&retval)) {
            zval_ptr_dtor(&retval);
            if (bucket->key) {
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
    if (EXPECTED(elements)) {
        ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
        RETURN_NEW_COLLECTION(elements_arr);
    }
    ARRAY_NEW(collection, 0);
    RETVAL_NEW_COLLECTION(collection);
}

PHP_METHOD(Collection, intersect)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW(intersected, 8);
    equal_check_func_t eql = NULL;
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        if (UNEXPECTED(eql == NULL)) {
            eql = equal_check_func_init(&bucket->val);
        }
        if (bucket->key) {
            zval* result = zend_hash_find(elements_arr, bucket->key);
            if (result && eql(&bucket->val, result)) {
                zend_hash_add(intersected, bucket->key, result);
                Z_TRY_ADDREF_P(result);
            }
        } else {
            zval* result = zend_hash_index_find(elements_arr, bucket->h);
            if (result && eql(&bucket->val, result)) {
                zend_hash_index_add(intersected, bucket->h, result);
                Z_TRY_ADDREF_P(result);
            }
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(intersected);
}

PHP_METHOD(Collection, intersectKeys)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW(intersected, 8);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        if (bucket->key) {
            if (zend_hash_exists(elements_arr, bucket->key)) {
                Z_TRY_ADDREF(bucket->val);
                zend_hash_add(intersected, bucket->key, &bucket->val);
            }
        } else {
            if (zend_hash_index_exists(elements_arr, bucket->h)) {
                Z_TRY_ADDREF(bucket->val);
                zend_hash_index_add(intersected, bucket->h, &bucket->val);
            }
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(intersected);
}

PHP_METHOD(Collection, intersectValues)
{

}

PHP_METHOD(Collection, isEmpty)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    RETVAL_BOOL(zend_hash_num_elements(current) == 0);
}

PHP_METHOD(Collection, isPacked)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    RETVAL_BOOL(HT_IS_PACKED(current));
}

PHP_METHOD(Collection, keys)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        zval val;
        if (bucket->key) {
            GC_ADDREF(bucket->key);
            ZVAL_STR(&val, bucket->key);
        } else {
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
    if (zend_hash_num_elements(current) == 0) {
        RETURN_NULL();
    }
    if (EX_NUM_ARGS() == 0) {
        uint32_t pos = current->nNumUsed;
        while (pos <= current->nNumUsed && Z_ISUNDEF(current->arData[pos].val)) {
            --pos;
        }
        RETURN_ZVAL(&current->arData[pos].val, 1, 0);
    }
    INIT_FCI(&fci, 2);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval)) {
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
        if (eql(element, &bucket->val)) {
            if (bucket->key) {
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
    if (EXPECTED(max)) {
        RETURN_ZVAL(max, 1, 0);
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
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&retval, 0, flags);
        }
        zend_hash_index_add(max_by, bucket - current->arData, &retval);
    ZEND_HASH_FOREACH_END();
    zval* max = zend_hash_minmax(max_by, cmp, 1);
    if (EXPECTED(max)) {
        zend_ulong offset = *(zend_ulong*)(max + 1);
        zval* ret = &(current->arData + offset)->val;
        RETVAL_ZVAL(ret, 1, 0);
    } else {
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
    zend_array* current = COLLECTION_FETCH_CURRENT();
    FCI_G = &fci;
    FCC_G = &fcc;
    ARRAY_CLONE(max_with, current);
    ZEND_HASH_FOREACH_BUCKET(max_with, Bucket* bucket)
        zend_object* obj = create_pair_obj();
        bucket_to_pair(obj, bucket);
        ZVAL_OBJ(&bucket->val, obj);
    ZEND_HASH_FOREACH_END();
    zval* result = zend_hash_minmax(max_with, bucket_compare_userland, 1);
    if (EXPECTED(result)) {
        RETVAL_ZVAL(PAIR_SECOND(Z_OBJ_P(result)), 1, 0);
    } else {
        RETVAL_NULL();
    }
    zend_array_destroy(max_with);
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
    if (EXPECTED(min)) {
        RETURN_ZVAL(min, 1, 0);
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
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&retval, 0, flags);
        }
        zend_hash_index_add(min_by, bucket - current->arData, &retval);
    ZEND_HASH_FOREACH_END();
    zval* min = zend_hash_minmax(min_by, cmp, 0);
    if (EXPECTED(min)) {
        zend_ulong offset = *(zend_ulong*)(min + 1);
        zval* ret = &(current->arData + offset)->val;
        RETVAL_ZVAL(ret, 1, 0);
    } else {
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
    zend_array* current = COLLECTION_FETCH_CURRENT();
    FCI_G = &fci;
    FCC_G = &fcc;
    ARRAY_CLONE(min_with, current);
    ZEND_HASH_FOREACH_BUCKET(min_with, Bucket* bucket)
        zend_object* obj = create_pair_obj();
        bucket_to_pair(obj, bucket);
        ZVAL_OBJ(&bucket->val, obj);
    ZEND_HASH_FOREACH_END();
    zval* result = zend_hash_minmax(min_with, bucket_compare_userland, 0);
    if (EXPECTED(result)) {
        RETVAL_ZVAL(PAIR_SECOND(Z_OBJ_P(result)), 1, 0);
    } else {
        RETVAL_NULL();
    }
    zend_array_destroy(min_with);
}

PHP_METHOD(Collection, minus)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW(new_collection, 8);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        zval* result;
        if (bucket->key) {
            result = zend_hash_find(elements_arr, bucket->key);
        } else {
            result = zend_hash_index_find(elements_arr, bucket->h);
        }
        if (result && fast_equal_check_function(result, &bucket->val)) {
            continue;
        }
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_add(new_collection, bucket->key, &bucket->val);
        } else {
            zend_hash_index_add(new_collection, bucket->h, &bucket->val);
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
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
        if (zend_is_true(&retval)) {
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

PHP_METHOD(Collection, partition)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    INIT_FCI(&fci, 2);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_object* pair = create_pair_obj();
    ARRAY_NEW(first_arr, 8);
    ARRAY_NEW(second_arr, 8);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zend_array* which = zend_is_true(&retval) ? first_arr : second_arr;
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_add(which, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(which, &bucket->val);
        } else {
            zend_hash_index_add(which, bucket->h, &bucket->val);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    zval first, second;
    ZVAL_ARR(&first, first_arr);
    ZVAL_ARR(&second, second_arr);
    pair_update_first(pair, &first);
    pair_update_second(pair, &second);
    RETVAL_OBJ(pair);
}

PHP_METHOD(Collection, plus)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_CLONE(new_collection, current);
    zend_bool packed = HT_IS_PACKED(current) && HT_IS_PACKED(elements_arr);
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_update(new_collection, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(new_collection, &bucket->val);
        } else {
            zend_hash_index_update(new_collection, bucket->h, &bucket->val);
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
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
    zend_bool packed = HT_IS_PACKED(current) && HT_IS_PACKED(elements_arr);
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_update(current, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(current, &bucket->val);
        } else {
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
    if (zend_hash_num_elements(current) == 0) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    INIT_FCI(&fci, 3);
    Bucket* bucket = current->arData;
    Bucket* end = bucket + current->nNumUsed;
    for (; bucket < end; ++bucket) {
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        ZVAL_COPY(&retval, &(bucket++)->val);
        break;
    }
    for (; bucket < end; ++bucket) {
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        ZVAL_COPY_VALUE(&params[0], &retval);
        ZVAL_COPY_VALUE(&params[1], &bucket->val);
        if (bucket->key) {
            ZVAL_STR(&params[2], bucket->key);
        } else {
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
    if (zend_hash_num_elements(current) == 0) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    INIT_FCI(&fci, 3);
    Bucket* start = current->arData;
    Bucket* bucket = start + current->nNumUsed - 1;
    for (; bucket >= start; --bucket) {
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        ZVAL_COPY(&retval, &(bucket--)->val);
        break;
    }
    for (; bucket >= start; --bucket) {
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        ZVAL_COPY_VALUE(&params[0], &retval);
        ZVAL_COPY_VALUE(&params[1], &bucket->val);
        if (bucket->key) {
            ZVAL_STR(&params[2], bucket->key);
        } else {
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
    if (Z_TYPE_P(key) == IS_LONG) {
        if (value == NULL) {
            RETURN_BOOL(zend_hash_index_del(current, Z_LVAL_P(key)) == SUCCESS);
        }
        found = zend_hash_index_find(current, Z_LVAL_P(key));
        if (found == NULL || fast_equal_check_function(found, value) == 0) {
            RETURN_FALSE;
        }
        RETURN_BOOL(zend_hash_index_del(current, Z_LVAL_P(key)) == SUCCESS);
    }
    if (Z_TYPE_P(key) == IS_STRING) {
        if (value == NULL) {
            RETURN_BOOL(zend_hash_del(current, Z_STR_P(key)) == SUCCESS);
        }
        found = zend_hash_find(current, Z_STR_P(key));
        if (found == NULL || fast_equal_check_function(found, value) == 0) {
            RETURN_FALSE;
        }
        RETURN_BOOL(zend_hash_del(current, Z_STR_P(key)) == SUCCESS);
    }
    ERR_BAD_KEY_TYPE();
    RETVAL_FALSE;
}

PHP_METHOD(Collection, removeAll)
{

}

PHP_METHOD(Collection, removeWhile)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    SEPARATE_CURRENT_COLLECTION(current);
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval)) {
            zend_hash_del_bucket(current, bucket);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
}

PHP_METHOD(Collection, retainAll)
{
    
}

PHP_METHOD(Collection, reverse)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(reversed, current);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_add(reversed, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(reversed, &bucket->val);
        } else {
            zend_hash_index_add(reversed, bucket->h, &bucket->val);
        }
    ZEND_HASH_FOREACH_END();
    if (GC_REFCOUNT(current) > 1) {
        GC_DELREF(current);
    } else {
        zend_array_destroy(current);
    }
    COLLECTION_FETCH_CURRENT() = reversed;
}

PHP_METHOD(Collection, reversed)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(reversed, current);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_add(reversed, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(reversed, &bucket->val);
        } else {
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
        Z_TRY_ADDREF_P(val);
        zend_hash_next_index_insert(shuffled, val);
    ZEND_HASH_FOREACH_END();
    size_t offset = 0;
    Bucket* bucket = shuffled->arData;
    for (; offset < num_elements - 1; ++offset) {
        zend_long rand_idx = php_mt_rand_range(offset, num_elements - 1);
        zend_hash_bucket_renum_swap(&bucket[offset], &bucket[rand_idx]);
    }
    if (GC_REFCOUNT(current) > 1) {
        GC_DELREF(current);
    } else {
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
        Z_TRY_ADDREF_P(val);
        zend_hash_next_index_insert(shuffled, val);
    ZEND_HASH_FOREACH_END();
    size_t offset = 0;
    Bucket* bucket = shuffled->arData;
    for (; offset < num_elements - 1; ++offset) {
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
    if (Z_TYPE_P(key) == IS_STRING) {
        SEPARATE_CURRENT_COLLECTION(current);
        zend_hash_update(current, Z_STR_P(key), value);
    } else if (Z_TYPE_P(key) == IS_LONG) {
        SEPARATE_CURRENT_COLLECTION(current);
        zend_hash_index_update(current, Z_LVAL_P(key), value);
    } else {
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
        if (zend_is_true(&retval)) {
            if (Z_TYPE(single) == IS_UNDEF) {
                ZVAL_COPY_VALUE(&single, &bucket->val);
            } else {
                RETURN_NULL();
            }
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    if (Z_TYPE(single) == IS_UNDEF) {
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
    ARRAY_NEW_EX(sliced, elements_arr);
    ZEND_HASH_FOREACH_VAL(elements_arr, zval* val)
        if (Z_TYPE_P(val) == IS_LONG) {
            zval* found = zend_hash_index_find(current, Z_LVAL_P(val));
            if (found) {
                Z_TRY_ADDREF_P(found);
                zend_hash_next_index_insert(sliced, found);
            }
        } else if (Z_TYPE_P(val) == IS_STRING) {
            zval* found = zend_hash_find(current, Z_STR_P(val));
            if (found) {
                Z_TRY_ADDREF_P(found);
                zend_hash_add(sliced, Z_STR_P(val), found);
            }
        } else {
            ERR_BAD_KEY_TYPE();
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(sliced);
}

PHP_METHOD(Collection, sort)
{
    zend_long flags = 0;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    SEPARATE_CURRENT_COLLECTION(current);
    compare_func_t cmp;
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        cmp = compare_func_init(val, 0, flags);
        break;
    ZEND_HASH_FOREACH_END();
    zend_hash_sort(current, cmp, 1);
}

PHP_METHOD(Collection, sortBy)
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
    SEPARATE_CURRENT_COLLECTION(current);
    uint32_t num_elements = zend_hash_num_elements(current);
    zval* sort_by = (zval*)malloc(num_elements * sizeof(zval));
    INIT_FCI(&fci, 2);
    compare_func_t cmp = NULL;
    uint32_t idx = 0;
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&retval, 0, flags);
        }
        if (bucket->key) {
            zend_string_release(bucket->key);
            bucket->key = NULL;
        }
        bucket->h = idx;
        ZVAL_COPY_VALUE(&sort_by[idx++], &retval);
    ZEND_HASH_FOREACH_END();
    COLLECTIONS_G(ref) = sort_by;
    COLLECTIONS_G(cmp) = cmp;
    zend_hash_sort_by(current);
    for (idx = 0; idx < num_elements; ++idx) {
        zval_ptr_dtor(&sort_by[idx]);
    }
    free(sort_by);
}

PHP_METHOD(Collection, sortByDescending)
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
    SEPARATE_CURRENT_COLLECTION(current);
    uint32_t num_elements = zend_hash_num_elements(current);
    zval* sort_by = (zval*)malloc(num_elements * sizeof(zval));
    INIT_FCI(&fci, 2);
    compare_func_t cmp = NULL;
    uint32_t idx = 0;
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&retval, 1, flags);
        }
        if (bucket->key) {
            zend_string_release(bucket->key);
            bucket->key = NULL;
        }
        bucket->h = idx;
        ZVAL_COPY_VALUE(&sort_by[idx++], &retval);
    ZEND_HASH_FOREACH_END();
    COLLECTIONS_G(ref) = sort_by;
    COLLECTIONS_G(cmp) = cmp;
    zend_hash_sort_by(current);
    for (idx = 0; idx < num_elements; ++idx) {
        zval_ptr_dtor(&sort_by[idx]);
    }
    free(sort_by);
}

PHP_METHOD(Collection, sortDescending)
{
    zend_long flags = 0;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    SEPARATE_CURRENT_COLLECTION(current);
    compare_func_t cmp;
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        cmp = compare_func_init(val, 1, flags);
        break;
    ZEND_HASH_FOREACH_END();
    zend_hash_sort(current, cmp, 1);
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
    ARRAY_CLONE(sorted_with, current);
    ZEND_HASH_FOREACH_BUCKET(sorted_with, Bucket* bucket)
        zend_object* obj = create_pair_obj();
        bucket_to_pair(obj, bucket);
        ZVAL_OBJ(&bucket->val, obj);
    ZEND_HASH_FOREACH_END();
    zend_hash_sort(sorted_with, bucket_compare_userland, 1);
    ZEND_HASH_FOREACH_VAL(sorted_with, zval* val)
        zend_object* pair = Z_OBJ_P(val);
        ZVAL_COPY_VALUE(val, PAIR_SECOND(pair));
        GC_DELREF(pair);
    ZEND_HASH_FOREACH_END();
    if (GC_REFCOUNT(current) > 1) {
        GC_DELREF(current);
    } else {
        zend_array_destroy(current);
    }
    COLLECTION_FETCH_CURRENT() = sorted_with;
}

PHP_METHOD(Collection, sorted)
{
    zend_long flags = 0;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_CLONE(sorted, current);
    compare_func_t cmp;
    ZEND_HASH_FOREACH_VAL(sorted, zval* val)
        cmp = compare_func_init(val, 0, flags);
        break;
    ZEND_HASH_FOREACH_END();
    zend_hash_sort(sorted, cmp, 1);
    RETVAL_NEW_COLLECTION(sorted);
}

PHP_METHOD(Collection, sortedBy)
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
    ARRAY_CLONE(sorted, current);
    uint32_t num_elements = zend_hash_num_elements(current);
    zval* sort_by = (zval*)malloc(num_elements * sizeof(zval));
    INIT_FCI(&fci, 2);
    compare_func_t cmp = NULL;
    uint32_t idx = 0;
    ZEND_HASH_FOREACH_BUCKET(sorted, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&retval, 0, flags);
        }
        if (bucket->key) {
            zend_string_release(bucket->key);
            bucket->key = NULL;
        }
        bucket->h = idx;
        ZVAL_COPY_VALUE(&sort_by[idx++], &retval);
    ZEND_HASH_FOREACH_END();
    COLLECTIONS_G(ref) = sort_by;
    COLLECTIONS_G(cmp) = cmp;
    zend_hash_sort_by(sorted);
    for (idx = 0; idx < num_elements; ++idx) {
        zval_ptr_dtor(&sort_by[idx]);
    }
    free(sort_by);
    RETVAL_NEW_COLLECTION(sorted);
}

PHP_METHOD(Collection, sortedByDescending)
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
    ARRAY_CLONE(sorted, current);
    uint32_t num_elements = zend_hash_num_elements(current);
    zval* sort_by = (zval*)malloc(num_elements * sizeof(zval));
    INIT_FCI(&fci, 2);
    compare_func_t cmp = NULL;
    uint32_t idx = 0;
    ZEND_HASH_FOREACH_BUCKET(sorted, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&retval, 1, flags);
        }
        if (bucket->key) {
            zend_string_release(bucket->key);
            bucket->key = NULL;
        }
        bucket->h = idx;
        ZVAL_COPY_VALUE(&sort_by[idx++], &retval);
    ZEND_HASH_FOREACH_END();
    COLLECTIONS_G(ref) = sort_by;
    COLLECTIONS_G(cmp) = cmp;
    zend_hash_sort_by(sorted);
    for (idx = 0; idx < num_elements; ++idx) {
        zval_ptr_dtor(&sort_by[idx]);
    }
    free(sort_by);
    RETVAL_NEW_COLLECTION(sorted);
}

PHP_METHOD(Collection, sortedDescending)
{
    zend_long flags = 0;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_CLONE(sorted, current);
    compare_func_t cmp;
    ZEND_HASH_FOREACH_VAL(sorted, zval* val)
        cmp = compare_func_init(val, 1, flags);
        break;
    ZEND_HASH_FOREACH_END();
    zend_hash_sort(sorted, cmp, 1);
    RETVAL_NEW_COLLECTION(sorted);
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
    ARRAY_CLONE(sorted_with, current);
    ZEND_HASH_FOREACH_BUCKET(sorted_with, Bucket* bucket)
        zend_object* obj = create_pair_obj();
        bucket_to_pair(obj, bucket);
        ZVAL_OBJ(&bucket->val, obj);
    ZEND_HASH_FOREACH_END();
    zend_hash_sort(sorted_with, bucket_compare_userland, 1);
    ZEND_HASH_FOREACH_VAL(sorted_with, zval* val)
        zend_object* pair = Z_OBJ_P(val);
        ZVAL_COPY_VALUE(val, PAIR_SECOND(pair));
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
    if (n < 0) {
        ERR_BAD_SIZE();
        return;
    }
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_long num_elements = zend_hash_num_elements(current);
    ARRAY_NEW(new_collection, n > num_elements ? num_elements : n);
    zend_bool packed = HT_IS_PACKED(current);
    Bucket* bucket = current->arData;
    Bucket* end = bucket + current->nNumUsed;
    for (; n > 0 && bucket < end; ++bucket) {
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        --n;
        Z_TRY_ADDREF(bucket->val);
        // Works for any zend_array, however, it doesn't make sense if you use any of these methods
        // on non-packed zend_arrays.
        if (bucket->key)  {
            zend_hash_add_new(new_collection, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(new_collection, &bucket->val);
        } else {
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
    if (n < 0) {
        ERR_BAD_SIZE();
        return;
    }
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_long num_elements = zend_hash_num_elements(current);
    ARRAY_NEW(new_collection, n > num_elements ? num_elements : n);
    zend_bool packed = HT_IS_PACKED(current);
    uint32_t idx = current->nNumUsed;
    zend_long num_taken = n;
    Bucket** taken = (Bucket**)malloc(num_taken * sizeof(Bucket*));
    // Note that the original element orders should be preserved as in kotlin.
    for (; num_taken > 0 && idx > 0; --idx) {
        Bucket* bucket = current->arData + idx - 1;
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        taken[--num_taken] = bucket;
    }
    memset(&taken[0], 0, num_taken * sizeof(Bucket*));
    int i = 0;
    for (; i < n; ++i) {
        Bucket* bucket = taken[i];
        if (bucket == NULL) {
            continue;
        }
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_add_new(new_collection, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(new_collection, &bucket->val);
        } else {
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
    ARRAY_NEW(new_collection, 8);
    zend_bool packed = HT_IS_PACKED(current);
    uint32_t num_elements = zend_hash_num_elements(current);
    Bucket** taken = (Bucket**)malloc(num_elements * sizeof(Bucket*));
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval)) {
            zval_ptr_dtor(&retval);
            taken[--num_elements] = bucket;
        } else {
            zval_ptr_dtor(&retval);
            break;
        }
    ZEND_HASH_FOREACH_END();
    memset(&taken[0], 0, num_elements * sizeof(Bucket*));
    int i = 0;
    for (; i < zend_hash_num_elements(current); ++i) {
        Bucket* bucket = taken[i];
        if (bucket == NULL) {
            continue;
        }
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_add_new(new_collection, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(new_collection, &bucket->val);
        } else {
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
    ARRAY_NEW(new_collection, 8);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval)) {
            zval_ptr_dtor(&retval);
            if (bucket->key) {
                zend_hash_add_new(new_collection, bucket->key, &bucket->val);
            } else if (packed) {
                zend_hash_next_index_insert(new_collection, &bucket->val);
            } else {
                zend_hash_index_add_new(new_collection, bucket->h, &bucket->val);
            }
        } else {
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
    zend_bool packed = HT_IS_PACKED(current) && HT_IS_PACKED(dest_arr);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        zval* result;
        if (bucket->key) {
            result = zend_hash_add(dest_arr, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(dest_arr, &bucket->val);
        } else {
            result = zend_hash_index_add(dest_arr, bucket->h, &bucket->val);
        }
        if (result) {
            Z_TRY_ADDREF(bucket->val);
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, toPairs)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        zend_object* obj = create_pair_obj();
        Z_TRY_ADDREF(bucket->val);
        bucket_to_pair(obj, bucket);
        zval pair;
        ZVAL_OBJ(&pair, obj);
        zend_hash_next_index_insert(new_collection, &pair);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, union)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = COLLECTION_FETCH_CURRENT();
    zend_bool packed = HT_IS_PACKED(current) && HT_IS_PACKED(elements_arr);
    ARRAY_CLONE(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        Z_TRY_ADDREF(bucket->val);
        if (bucket->key) {
            zend_hash_add(new_collection, bucket->key, &bucket->val);
        } else if (packed) {
            zend_hash_next_index_insert(new_collection, &bucket->val);
        } else {
            zend_hash_index_add(new_collection, bucket->h, &bucket->val);
        }
    ZEND_HASH_FOREACH_END();
    uint32_t num_elements = zend_hash_num_elements(new_collection);
    compare_func_t cmp = NULL;
    equal_check_func_t eql = NULL;
    Bucket* ref = (Bucket*)malloc(num_elements * sizeof(Bucket));
    uint32_t idx = 0;
    ZEND_HASH_FOREACH_BUCKET(new_collection, Bucket* bucket)
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&bucket->val, 0, 0);
            eql = equal_check_func_init(&bucket->val);
        }
        Bucket* dest = &ref[idx++];
        dest->key = NULL;
        dest->h = bucket - new_collection->arData;
        memcpy(&dest->val, &bucket->val, sizeof(zval));
    ZEND_HASH_FOREACH_END();
    COLLECTIONS_G(cmp) = cmp;
    array_distinct(new_collection, ref, cmp, eql);
    free(ref);
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, values)
{
    zend_array* current = COLLECTION_FETCH_CURRENT();
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        Z_TRY_ADDREF_P(val);
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
    Z_TRY_ADDREF_P(first);
    Z_TRY_ADDREF_P(second);
    pair_update_first(Z_OBJ_P(getThis()), first);
    pair_update_second(Z_OBJ_P(getThis()), second);
}
