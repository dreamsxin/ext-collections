//
// ext-collections/collections_methods.c
//
// @Author CismonX
//

#include "php_collections.h"
#include "php_collections_me.h"

#include <ext/standard/php_string.h>
#include <ext/standard/php_mt_rand.h>

#if PHP_VERSION_ID < 70300
#define GC_ADDREF(p)             ++GC_REFCOUNT(p)
#define GC_DELREF(p)             --GC_REFCOUNT(p)
#endif

#define FCI_G                    COLLECTIONS_G(fci)
#define FCC_G                    COLLECTIONS_G(fcc)
#define REF_G                    COLLECTIONS_G(ref)
#define CMP_G                    COLLECTIONS_G(cmp)

#define Z_COLLECTION_P(val)      (Z_OBJ_P(val)->properties)
#define THIS_COLLECTION          Z_COLLECTION_P(getThis())
#define PAIR_FIRST(obj)          OBJ_PROP_NUM(obj, 0)
#define PAIR_SECOND(obj)         OBJ_PROP_NUM(obj, 1)

#define IS_COLLECTION(val)                                                 \
    Z_TYPE(val) == IS_OBJECT && Z_OBJCE(val) == collections_collection_ce
#define IS_PAIR(val)                                                       \
    Z_TYPE(val) == IS_OBJECT && Z_OBJCE(val) == collections_pair_ce

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
    (fci)->param_count = (num_args);                                       \
    (fci)->retval = &retval;                                               \
    (fci)->params = params

#define CALLBACK_KEYVAL_INVOKE(params, bucket)                             \
    ZVAL_COPY_VALUE(&(params)[0], &(bucket)->val);                         \
    if ((bucket)->key) {                                                   \
        ZVAL_STR(&(params)[1], (bucket)->key);                             \
    } else {                                                               \
        ZVAL_LONG(&(params)[1], (bucket)->h);                              \
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
#define ERR_NOT_PACKED()                                                   \
    PHP_COLLECTIONS_ERROR(E_WARNING, "The array should be packed")
#define ERR_SILENCED()

#define ELEMENTS_VALIDATE(elements, err, err_then)                         \
    zend_array* elements##_arr;                                            \
    if (IS_COLLECTION(*elements)) {                                        \
        (elements##_arr) = Z_COLLECTION_P(elements);                       \
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

#define RETVAL_NEW_COLLECTION(ht)                                          \
    {                                                                      \
        zend_object* _obj = create_collection_obj();                       \
        if (GC_REFCOUNT(ht) > 1) {                                         \
            GC_ADDREF(ht);                                                 \
        }                                                                  \
        _obj->properties = ht;                                             \
        RETVAL_OBJ(_obj);                                                  \
    }

#define RETURN_NEW_COLLECTION(ht)                                          \
    {                                                                      \
        RETVAL_NEW_COLLECTION(ht);                                         \
        return;                                                            \
    }

typedef int (*equal_check_func_t)(zval*, zval*);

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

static zend_always_inline void array_add_bucket(zend_array* ht, Bucket* bucket,
    zend_bool packed)
{
    zval* result;
    if (bucket->key) {
        result = zend_hash_add(ht, bucket->key, &bucket->val);
    } else if (packed) {
        result = zend_hash_next_index_insert(ht, &bucket->val);
    } else {
        result = zend_hash_index_add(ht, bucket->h, &bucket->val);
    }
    if (result) {
        Z_TRY_ADDREF(bucket->val);
    }
}

static zend_always_inline void array_add_bucket_new(zend_array* ht, Bucket* bucket,
    zend_bool packed)
{
    Z_TRY_ADDREF(bucket->val);
    if (bucket->key) {
        zend_hash_add_new(ht, bucket->key, &bucket->val);
    } else if (packed) {
        zend_hash_next_index_insert(ht, &bucket->val);
    } else {
        zend_hash_index_add_new(ht, bucket->h, &bucket->val);
    }
}

static zend_always_inline void array_update_bucket(zend_array* ht, Bucket* bucket,
    zend_bool packed)
{
    Z_TRY_ADDREF(bucket->val);
    if (bucket->key) {
        zend_hash_update(ht, bucket->key, &bucket->val);
    } else if (packed) {
        zend_hash_next_index_insert(ht, &bucket->val);
    } else {
        zend_hash_index_update(ht, bucket->h, &bucket->val);
    }
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

static zend_always_inline void array_release(zend_array* ht)
{
    if (GC_REFCOUNT(ht) > 1) {
        GC_DELREF(ht);
    } else {
        zend_array_destroy(ht);
    }
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
    zval* ref = REF_G;
    return CMP_G(&ref[b1->h], &ref[b2->h]);
}

static int bucket_compare_with_idx(const void* op1, const void* op2)
{
    zend_long h1 = ((Bucket*)op1)->h;
    zend_long h2 = ((Bucket*)op2)->h;
    int result = CMP_G(op1, op2);
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

static zend_always_inline zend_long binary_search(Bucket* ref, zval* val,
    zend_long from_idx, zend_long to_idx, compare_func_t cmp)
{
    zend_long low = from_idx;
    zend_long high = to_idx - 1;
    while (low <= high) {
        zend_long mid = (low + high) >> 1;
        int result = cmp(&ref[mid].val, val);
        if (result == 1) {
            high = mid - 1;
        } else if (result == -1) {
            low = mid + 1;
        } else {
            return mid;
        }
    }
    return -(low + 1);
}

static zend_always_inline void array_sort_by(zend_array* ht)
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

static zend_always_inline void array_packed_renumber(zend_array* ht)
{
    Bucket* bucket = ht->arData;
    Bucket* last = NULL;
    uint32_t idx;
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
}

static zend_always_inline void delete_by_offset(zend_array* ht, uint32_t offset,
    zend_bool packed)
{
    Bucket* bucket = ht->arData + offset;
    if (packed) {
        zval_ptr_dtor(&bucket->val);
        ZVAL_UNDEF(&bucket->val);
        --ht->nNumOfElements;
    } else {
        zend_hash_del_bucket(ht, bucket);
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
    for (idx = 1; idx < num_elements; ++idx) {
        Bucket* bucket = &ref[idx];
        if (eql(&bucket->val, &first->val)) {
            delete_by_offset(ht, bucket->h, packed);
        } else {
            first = bucket;
        }
    }
    if (packed) {
        array_packed_renumber(ht);
    }
}

static zend_always_inline uint32_t advance_idx(zend_array* ht, Bucket* ref, uint32_t offset,
    uint32_t max_offset, equal_check_func_t eql, zend_bool del_dup, zend_bool packed)
{
    for (++offset; offset < max_offset; ++offset) {
        if (!eql(&ref[offset].val, &ref[offset - 1].val)) {
            return offset;
        }
        if (del_dup) {
            delete_by_offset(ht, ref[offset].h, packed);
        }
    }
    return 0;
}

static zend_always_inline void tail_cleanup(zend_array* ht,
    Bucket* ref, uint32_t offset, uint32_t max_offset, zend_bool packed,
    equal_check_func_t eql, zend_bool subtract, zend_bool del_dup)
{
    if (subtract) {
        if (!del_dup) {
            return;
        }
        while (offset) {
            offset = advance_idx(ht, ref, offset, max_offset, eql, 1, packed);
        }
    } else {
        for (; offset < max_offset; ++offset) {
            delete_by_offset(ht, ref[offset].h, packed);
        }
    }
}

static zend_always_inline void array_slice_by(zend_array* ht, zend_array* other,
    zend_bool del_dup, zend_bool subtract)
{
    zend_bool packed = HT_IS_PACKED(ht);
    uint32_t num_this = zend_hash_num_elements(ht);
    if (UNEXPECTED(num_this == 0)) {
        return;
    }
    uint32_t num_other = zend_hash_num_elements(other);
    if (UNEXPECTED(num_other == 0)) {
        if (!subtract) {
            zend_hash_clean(ht);
        }
        return;
    }
    Bucket* ref_this = (Bucket*)malloc(num_this * sizeof(Bucket));
    compare_func_t cmp = NULL;
    equal_check_func_t eql;
    uint32_t idx = 0;
    ZEND_HASH_FOREACH_BUCKET(ht, Bucket* bucket)
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&bucket->val, 0, 0);
            eql = equal_check_func_init(&bucket->val);
        }
        Bucket* dest = &ref_this[idx++];
        dest->key = NULL;
        dest->h = bucket - ht->arData;
        memcpy(&dest->val, &bucket->val, sizeof(zval));
    ZEND_HASH_FOREACH_END();
    CMP_G = cmp;
    zend_sort(ref_this, num_this, sizeof(Bucket), packed ? bucket_compare_with_idx : cmp,
        (swap_func_t)zend_hash_bucket_packed_swap);
    Bucket* ref_other = (Bucket*)malloc(num_other * sizeof(Bucket));
    uint32_t idx_other = 0;
    ZEND_HASH_FOREACH_BUCKET(other, Bucket* bucket)
        Bucket* dest = &ref_other[idx_other++];
        dest->key = NULL;
        dest->h = bucket - other->arData;
        memcpy(&dest->val, &bucket->val, sizeof(zval));
    ZEND_HASH_FOREACH_END();
    zend_sort(ref_other, num_other, sizeof(Bucket), packed ? bucket_compare_with_idx : cmp,
        (swap_func_t)zend_hash_bucket_packed_swap);
    idx = idx_other = 0;
    while (1) {
        Bucket* this = &ref_this[idx];
        Bucket* other = &ref_other[idx_other];
        int result = cmp(&this->val, &other->val);
        if (result == 0) {
            // Element exists in both zend_array.
            if (subtract) {
                delete_by_offset(ht, ref_this[idx].h, packed);
            }
            idx = advance_idx(ht, ref_this, idx, num_this, eql, subtract || del_dup, packed);
            if (UNEXPECTED(idx == 0)) {
                break;
            }
            idx_other = advance_idx(NULL, ref_other, idx_other, num_other, eql, 0, 0);
            if (UNEXPECTED(idx_other == 0)) {
                tail_cleanup(ht, ref_this, idx, num_this, packed, eql, subtract, del_dup);
                break;
            }
        } else if (result == -1) {
            // Element `ref_this[idx]` exists only in current zend_array.
            if (!subtract) {
                delete_by_offset(ht, ref_this[idx].h, packed);
            }
            idx = advance_idx(ht, ref_this, idx, num_this, eql, !subtract || del_dup, packed);
            if (UNEXPECTED(idx == 0)) {
                break;
            }
        } else {
            // Element `ref_other[other_idx]` exists only in the other zend_array.
            idx_other = advance_idx(NULL, ref_other, idx_other, num_other, eql, 0, 0);
            if (UNEXPECTED(idx_other == 0)) {
                tail_cleanup(ht, ref_this, idx, num_this, packed, eql, subtract, del_dup);
                break;
            }
        }
    }
    if (packed) {
        array_packed_renumber(ht);
    }
    free(ref_this);
    free(ref_other);
}

int count_collection(zval* obj, zend_long* count)
{
    zend_array* current = Z_COLLECTION_P(obj);
    *count = zend_hash_num_elements(current);
    return SUCCESS;
}

int collection_offset_exists(zval* object, zval* offset, int check_empty)
{
    zend_array* current = Z_COLLECTION_P(object);
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
    zend_array* current = Z_COLLECTION_P(object);
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
    zend_array* current = Z_COLLECTION_P(object);
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
    zend_array* current = Z_COLLECTION_P(object);
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
    zend_array* current = Z_COLLECTION_P(object);
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
    zend_array* current = THIS_COLLECTION;
    zend_bool packed = HT_IS_PACKED(current) && HT_IS_PACKED(elements_arr);
    SEPARATE_CURRENT_COLLECTION(current);
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        array_add_bucket(current, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
    zend_array* dest_arr = Z_COLLECTION_P(dest);
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
    zend_array* dest_arr = Z_COLLECTION_P(dest);
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
    zend_array* current = THIS_COLLECTION;
    double sum = 0;
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        if (Z_TYPE_P(val) == IS_LONG) {
            sum += Z_LVAL_P(val);
        } else if (Z_TYPE_P(val) == IS_DOUBLE) {
            sum += Z_DVAL_P(val);
        } else {
            ERR_NOT_NUMERIC();
            RETURN_NULL();
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_DOUBLE(sum / zend_hash_num_elements(current));
}

PHP_METHOD(Collection, binarySearch)
{
    zval* element;
    zend_long flags = 0;
    zend_long from_idx = 0;
    zend_array* current = THIS_COLLECTION;
    uint32_t num_elements = zend_hash_num_elements(current);
    zend_long to_idx = num_elements;
    ZEND_PARSE_PARAMETERS_START(1, 4)
        Z_PARAM_ZVAL(element)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
        Z_PARAM_LONG(from_idx)
        Z_PARAM_LONG(to_idx)
    ZEND_PARSE_PARAMETERS_END();
    if (UNEXPECTED(!HT_IS_PACKED(current))) {
        ERR_NOT_PACKED();
        RETURN_NULL();
    }
    if (UNEXPECTED(from_idx < 0)) {
        ERR_BAD_INDEX();
        RETURN_NULL();
    }
    if (to_idx > num_elements) {
        to_idx = num_elements;
    }
    if (UNEXPECTED(to_idx < from_idx)) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    compare_func_t cmp;
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        cmp = compare_func_init(val, 0, flags);
        break;
    ZEND_HASH_FOREACH_END();
    RETVAL_LONG(binary_search(current->arData, element, from_idx, to_idx, cmp));
}

PHP_METHOD(Collection, binarySearchBy)
{
    zval* element;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    zend_long flags = 0;
    zend_long from_idx = 0;
    zend_array* current = THIS_COLLECTION;
    uint32_t num_elements = zend_hash_num_elements(current);
    zend_long to_idx = num_elements;
    ZEND_PARSE_PARAMETERS_START(2, 5)
        Z_PARAM_ZVAL(element)
        Z_PARAM_FUNC(fci, fcc)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
        Z_PARAM_LONG(from_idx)
        Z_PARAM_LONG(to_idx)
    ZEND_PARSE_PARAMETERS_END();
    if (UNEXPECTED(!HT_IS_PACKED(current))) {
        ERR_NOT_PACKED();
        RETURN_NULL();
    }
    if (UNEXPECTED(from_idx < 0)) {
        ERR_BAD_INDEX();
        RETURN_NULL();
    }
    if (to_idx > num_elements) {
        to_idx = num_elements;
    }
    if (UNEXPECTED(to_idx < from_idx)) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    INIT_FCI(&fci, 2);
    uint32_t range = to_idx - from_idx;
    Bucket* ref = (Bucket*)malloc(range * sizeof(Bucket));
    compare_func_t cmp = NULL;
    uint32_t idx = 0;
    Bucket* bucket = current->arData + from_idx;
    Bucket* end = current->arData + to_idx;
    for (; bucket < end; ++bucket) {
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        memcpy(&ref[idx++].val, &retval, sizeof(zval));
        if (UNEXPECTED(cmp == NULL)) {
            cmp = compare_func_init(&retval, 0, flags);
        }
    }
    zend_long result = binary_search(ref, element, 0, range, cmp);
    RETVAL_LONG(result < 0 ? result - from_idx : result + from_idx);
    for (idx = 0; idx < range; ++idx) {
        zval_ptr_dtor(&ref[idx].val);
    }
    free(ref);
}

PHP_METHOD(Collection, chunked)
{
    zend_long size;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_LONG(size)
        Z_PARAM_OPTIONAL
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    if (size <= 0) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    zend_bool transform = EX_NUM_ARGS() > 1;
    zend_array* current = THIS_COLLECTION;
    zend_bool packed = HT_IS_PACKED(current);
    INIT_FCI(&fci, 2);
    ARRAY_NEW(chunked, 8);
    uint32_t num_remaining = 0;
    uint32_t num_chunks = 0;
    zend_array* chunk = NULL;
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        if (num_remaining == 0) {
            chunk = (zend_array*)emalloc(sizeof(zend_array));
            zend_hash_init(chunk, 8, NULL, ZVAL_PTR_DTOR, 0);
            num_remaining = size;
        }
        array_add_bucket_new(chunk, bucket, packed);
        if (--num_remaining == 0) {
            ZVAL_ARR(&params[0], chunk);
            if (transform) {
                ZVAL_LONG(&params[1], num_chunks++);
                zend_call_function(&fci, &fcc);
                array_release(chunk);
                zend_hash_next_index_insert(chunked, &retval);
            } else {
                zend_hash_next_index_insert(chunked, &params[0]);
            }
        }
    ZEND_HASH_FOREACH_END();
    if (num_remaining) {
        ZVAL_ARR(&params[0], chunk);
        if (transform) {
            ZVAL_LONG(&params[1], num_chunks++);
            zend_call_function(&fci, &fcc);
            array_release(chunk);
            zend_hash_next_index_insert(chunked, &retval);
        } else {
            zend_hash_next_index_insert(chunked, &params[0]);
        }
    }
    RETVAL_NEW_COLLECTION(chunked);
}

PHP_METHOD(Collection, containsAll)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_bool packed = HT_IS_PACKED(current);
    ARRAY_NEW(new_collection, new_size < num_elements ? new_size : num_elements);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        array_add_bucket_new(new_collection, bucket, packed);
        if (--new_size == 0) {
            break;
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, copyOfRange)
{
    zend_long from_idx, to_idx;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(from_idx)
        Z_PARAM_LONG(to_idx)
    ZEND_PARSE_PARAMETERS_END();
    if (UNEXPECTED(from_idx < 0)) {
        ERR_BAD_INDEX();
        RETURN_NULL();
    }
    if (UNEXPECTED(to_idx < from_idx)) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    zend_array* current = THIS_COLLECTION;
    uint32_t num_elements = to_idx - from_idx;
    ARRAY_NEW(new_collection, num_elements);
    zend_bool packed = HT_IS_PACKED(current);
    Bucket* bucket = current->arData;
    Bucket* end = bucket + current->nNumUsed;
    for (bucket += from_idx; num_elements > 0 && bucket < end; ++bucket) {
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        --num_elements;
        array_add_bucket_new(new_collection, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
    compare_func_t cmp = NULL;
    equal_check_func_t eql = NULL;
    uint32_t num_elements = zend_hash_num_elements(current);
    Bucket* ref = (Bucket*)malloc(num_elements * sizeof(Bucket));
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
    CMP_G = cmp;
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
    zend_array* current = THIS_COLLECTION;
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
    CMP_G = cmp;
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
    zend_array* current = THIS_COLLECTION;
    ARRAY_CLONE(new_collection, current);
    Bucket* bucket = new_collection->arData;
    Bucket* end = bucket + new_collection->nNumUsed;
    for (; n > 0 && bucket < end; ++bucket) {
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        --n;
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
    zend_array* current = THIS_COLLECTION;
    ARRAY_CLONE(new_collection, current);
    unsigned idx = new_collection->nNumUsed;
    for (; n > 0 && idx > 0; --idx) {
        Bucket* bucket = new_collection->arData + idx - 1;
        if (Z_ISUNDEF(bucket->val)) {
            continue;
        }
        --n;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
    zend_long to_idx = zend_hash_num_elements(current);
    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_ZVAL(element)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(from_idx)
        Z_PARAM_LONG(to_idx)
    ZEND_PARSE_PARAMETERS_END();
    if (UNEXPECTED(from_idx < 0)) {
        ERR_BAD_INDEX();
        RETURN_NULL();
    }
    if (UNEXPECTED(to_idx < from_idx)) {
        ERR_BAD_SIZE();
        RETURN_NULL();
    }
    SEPARATE_CURRENT_COLLECTION(current);
    uint32_t num_elements = to_idx - from_idx;
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
    zend_array* current = THIS_COLLECTION;
    ARRAY_NEW(new_collection, 8);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval)) {
            array_add_bucket_new(new_collection, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
    ARRAY_NEW(new_collection, 8);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval)) {
            array_add_bucket_new(new_collection, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
    zend_array* dest_arr = Z_COLLECTION_P(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval)) {
            array_add_bucket(dest_arr, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
    zend_array* dest_arr = Z_COLLECTION_P(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval)) {
            array_add_bucket(dest_arr, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zval* retval_p = &retval;
        ELEMENTS_VALIDATE(retval_p, ERR_BAD_CALLBACK_RETVAL, continue);
        ZEND_HASH_FOREACH_BUCKET(retval_p_arr, Bucket* bucket)
            array_add_bucket(new_collection, bucket, 1);
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
    zend_array* current = THIS_COLLECTION;
    zend_array* dest_arr = Z_COLLECTION_P(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zval* retval_p = &retval;
        ELEMENTS_VALIDATE(retval_p, ERR_BAD_CALLBACK_RETVAL, continue);
        ZEND_HASH_FOREACH_BUCKET(retval_p_arr, Bucket* bucket)
            array_add_bucket(dest_arr, bucket, 1);
        ZEND_HASH_FOREACH_END();
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, flatten)
{
    zend_array* current = THIS_COLLECTION;
    ARRAY_NEW_EX(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        zval* val = &bucket->val;
        ELEMENTS_VALIDATE(val, ERR_SILENCED, {
            if (bucket->key) {
                val = zend_hash_add(new_collection, bucket->key, val);
            } else {
                val = zend_hash_next_index_insert(new_collection, val);
            }
            if (val) {
                Z_TRY_ADDREF_P(val);
            }
            continue;
        });
        ZEND_HASH_FOREACH_BUCKET(val_arr, Bucket* bucket)
            array_add_bucket(new_collection, bucket, 1);
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
            zend_hash_add_new(group, bucket->key, value);
        } else if (packed) {
            zend_hash_next_index_insert(group, value);
        } else {
            zend_hash_index_add_new(group, bucket->h, value);
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
    zend_array* current = THIS_COLLECTION;
    zend_array* dest_arr = Z_COLLECTION_P(dest);
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
            value = zend_hash_next_index_insert(group, value);
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
    ARRAY_NEW(intersected, 8);
    equal_check_func_t eql = NULL;
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        if (UNEXPECTED(eql == NULL)) {
            eql = equal_check_func_init(&bucket->val);
        }
        if (bucket->key) {
            zval* result = zend_hash_find(elements_arr, bucket->key);
            if (result && eql(&bucket->val, result)) {
                Z_TRY_ADDREF_P(result);
                zend_hash_add_new(intersected, bucket->key, result);
            }
        } else {
            zval* result = zend_hash_index_find(elements_arr, bucket->h);
            if (result && eql(&bucket->val, result)) {
                Z_TRY_ADDREF_P(result);
                zend_hash_index_add_new(intersected, bucket->h, result);
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
    zend_array* current = THIS_COLLECTION;
    ARRAY_NEW(intersected, 8);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        if (bucket->key) {
            if (zend_hash_exists(elements_arr, bucket->key)) {
                Z_TRY_ADDREF(bucket->val);
                zend_hash_add_new(intersected, bucket->key, &bucket->val);
            }
        } else {
            if (zend_hash_index_exists(elements_arr, bucket->h)) {
                Z_TRY_ADDREF(bucket->val);
                zend_hash_index_add_new(intersected, bucket->h, &bucket->val);
            }
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(intersected);
}

PHP_METHOD(Collection, intersectValues)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = THIS_COLLECTION;
    ARRAY_CLONE(intersected, current);
    array_slice_by(intersected, elements_arr, 1, 0);
    RETVAL_NEW_COLLECTION(intersected);
}

PHP_METHOD(Collection, isEmpty)
{
    zend_array* current = THIS_COLLECTION;
    RETVAL_BOOL(zend_hash_num_elements(current) == 0);
}

PHP_METHOD(Collection, isPacked)
{
    zend_array* current = THIS_COLLECTION;
    RETVAL_BOOL(HT_IS_PACKED(current));
}

PHP_METHOD(Collection, keys)
{
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
    zend_array* dest_arr = Z_COLLECTION_P(dest);
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
            zend_hash_add_new(new_collection, bucket->key, &bucket->val);
        } else {
            zend_hash_index_add_new(new_collection, bucket->h, &bucket->val);
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
    zend_object* pair = create_pair_obj();
    ARRAY_NEW(first_arr, 8);
    ARRAY_NEW(second_arr, 8);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        zend_array* which = zend_is_true(&retval) ? first_arr : second_arr;
        array_add_bucket_new(which, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
    ARRAY_CLONE(new_collection, current);
    zend_bool packed = HT_IS_PACKED(current) && HT_IS_PACKED(elements_arr);
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        array_update_bucket(new_collection, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
    SEPARATE_CURRENT_COLLECTION(current);
    zend_bool packed = HT_IS_PACKED(current) && HT_IS_PACKED(elements_arr);
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        array_update_bucket(current, bucket, packed);
    ZEND_HASH_FOREACH_END();
}

PHP_METHOD(Collection, reduce)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = THIS_COLLECTION;
    SEPARATE_CURRENT_COLLECTION(current);
    array_slice_by(current, elements_arr, 0, 1);
}

PHP_METHOD(Collection, removeWhile)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = THIS_COLLECTION;
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
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = THIS_COLLECTION;
    SEPARATE_CURRENT_COLLECTION(current);
    array_slice_by(current, elements_arr, 0, 0);
}

PHP_METHOD(Collection, retainWhile)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = THIS_COLLECTION;
    SEPARATE_CURRENT_COLLECTION(current);
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (!zend_is_true(&retval)) {
            zend_hash_del_bucket(current, bucket);
        }
        zval_ptr_dtor(&retval);
    ZEND_HASH_FOREACH_END();
}

PHP_METHOD(Collection, reverse)
{
    zend_array* current = THIS_COLLECTION;
    ARRAY_NEW_EX(reversed, current);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        array_add_bucket_new(reversed, bucket, packed);
    ZEND_HASH_FOREACH_END();
    array_release(current);
    THIS_COLLECTION = reversed;
}

PHP_METHOD(Collection, reversed)
{
    zend_array* current = THIS_COLLECTION;
    ARRAY_NEW_EX(reversed, current);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_REVERSE_FOREACH_BUCKET(current, Bucket* bucket)
        array_add_bucket_new(reversed, bucket, packed);
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(reversed);
}

PHP_METHOD(Collection, shuffle)
{
    zend_array* current = THIS_COLLECTION;
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
    array_release(current);
    THIS_COLLECTION = shuffled;
}

PHP_METHOD(Collection, shuffled)
{
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
    if (Z_TYPE_P(key) == IS_STRING) {
        SEPARATE_CURRENT_COLLECTION(current);
        Z_TRY_ADDREF_P(value);
        zend_hash_update(current, Z_STR_P(key), value);
    } else if (Z_TYPE_P(key) == IS_LONG) {
        SEPARATE_CURRENT_COLLECTION(current);
        Z_TRY_ADDREF_P(value);
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    REF_G = sort_by;
    CMP_G = cmp;
    array_sort_by(current);
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
    zend_array* current = THIS_COLLECTION;
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
    REF_G = sort_by;
    CMP_G = cmp;
    array_sort_by(current);
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    array_release(current);
    THIS_COLLECTION = sorted_with;
}

PHP_METHOD(Collection, sorted)
{
    zend_long flags = 0;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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
    REF_G = sort_by;
    CMP_G = cmp;
    array_sort_by(sorted);
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
    zend_array* current = THIS_COLLECTION;
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
    REF_G = sort_by;
    CMP_G = cmp;
    array_sort_by(sorted);
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
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
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

PHP_METHOD(Collection, subtract)
{
    zval* elements;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(elements)
    ZEND_PARSE_PARAMETERS_END();
    ELEMENTS_VALIDATE(elements, ERR_BAD_ARGUMENT_TYPE, return);
    zend_array* current = THIS_COLLECTION;
    ARRAY_CLONE(subtracted, current);
    array_slice_by(subtracted, elements_arr, 1, 1);
    RETVAL_NEW_COLLECTION(subtracted);
}

PHP_METHOD(Collection, sum)
{
    zend_array* current = THIS_COLLECTION;
    zval sum;
    ZVAL_NULL(&sum);
    ZEND_HASH_FOREACH_VAL(current, zval* val)
        if (UNEXPECTED(ZVAL_IS_NULL(&sum))) {
            if (Z_TYPE_P(val) == IS_LONG) {
                ZVAL_LONG(&sum, 0);
            } else if (EXPECTED(Z_TYPE_P(val) == IS_DOUBLE)) {
                ZVAL_DOUBLE(&sum, 0.0);
            } else {
                ERR_NOT_NUMERIC();
                RETURN_NULL();
            }
        }
        if (Z_TYPE_P(val) == IS_LONG) {
            Z_LVAL(sum) += Z_LVAL_P(val);
        } else if (EXPECTED(Z_TYPE_P(val) == IS_DOUBLE)) {
            Z_DVAL(sum) += Z_DVAL_P(val);
        } else {
            ERR_NOT_NUMERIC();
            RETURN_NULL();
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(&sum, 0, 0);
}

PHP_METHOD(Collection, sumBy)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = THIS_COLLECTION;
    zval sum;
    ZVAL_NULL(&sum);
    INIT_FCI(&fci, 2);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (UNEXPECTED(ZVAL_IS_NULL(&sum))) {
            if (Z_TYPE(retval) == IS_LONG) {
                ZVAL_LONG(&sum, 0);
            } else if (EXPECTED(Z_TYPE(retval) == IS_DOUBLE)) {
                ZVAL_DOUBLE(&sum, 0.0);
            } else {
                ERR_NOT_NUMERIC();
                zval_ptr_dtor(&retval);
                RETURN_NULL();
            }
        }
        if (Z_TYPE(retval) == IS_LONG) {
            Z_LVAL(sum) += Z_LVAL(retval);
        } else if (EXPECTED(Z_TYPE(retval) == IS_DOUBLE)) {
            Z_DVAL(sum) += Z_DVAL(retval);
        } else {
            ERR_NOT_NUMERIC();
            zval_ptr_dtor(&retval);
            RETURN_NULL();
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(&sum, 0, 0);
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
    zend_array* current = THIS_COLLECTION;
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
        // Works for any zend_array, however, it doesn't make sense if you use any of
        // these methods on non-packed zend_arrays.
        array_add_bucket_new(new_collection, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
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
        array_add_bucket_new(new_collection, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
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
        array_add_bucket_new(new_collection, bucket, packed);
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
    zend_array* current = THIS_COLLECTION;
    ARRAY_NEW(new_collection, 8);
    zend_bool packed = HT_IS_PACKED(current);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        CALLBACK_KEYVAL_INVOKE(params, bucket);
        if (zend_is_true(&retval)) {
            zval_ptr_dtor(&retval);
            array_add_bucket_new(new_collection, bucket, packed);
        } else {
            zval_ptr_dtor(&retval);
            break;
        }
    ZEND_HASH_FOREACH_END();
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, toArray)
{
    zend_array* retval = THIS_COLLECTION;
    GC_ADDREF(retval);
    RETVAL_ARR(retval);
}

PHP_METHOD(Collection, toCollection)
{
    zval* dest;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(dest, collections_collection_ce)
    ZEND_PARSE_PARAMETERS_END();
    zend_array* current = THIS_COLLECTION;
    zend_array* dest_arr = Z_COLLECTION_P(dest);
    SEPARATE_COLLECTION(dest_arr, dest);
    zend_bool packed = HT_IS_PACKED(current) && HT_IS_PACKED(dest_arr);
    ZEND_HASH_FOREACH_BUCKET(current, Bucket* bucket)
        array_add_bucket(dest_arr, bucket, packed);
    ZEND_HASH_FOREACH_END();
    RETVAL_ZVAL(dest, 1, 0);
}

PHP_METHOD(Collection, toPairs)
{
    zend_array* current = THIS_COLLECTION;
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
    zend_array* current = THIS_COLLECTION;
    zend_bool packed = HT_IS_PACKED(current) && HT_IS_PACKED(elements_arr);
    ARRAY_CLONE(new_collection, current);
    ZEND_HASH_FOREACH_BUCKET(elements_arr, Bucket* bucket)
        array_add_bucket(new_collection, bucket, packed);
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
    CMP_G = cmp;
    array_distinct(new_collection, ref, cmp, eql);
    free(ref);
    RETVAL_NEW_COLLECTION(new_collection);
}

PHP_METHOD(Collection, values)
{
    zend_array* current = THIS_COLLECTION;
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
