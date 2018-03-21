//
// ext-collections/collections_fe.c
//
// @Author CismonX
//
#include <php.h>

#include "php_collections.h"
#include "php_collections_fe.h"

ZEND_BEGIN_ARG_INFO(action_arginfo, 0)
    ZEND_ARG_CALLABLE_INFO(0, action, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(element_arginfo, 0)
    ZEND_ARG_INFO(0, element)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(elements_arginfo, 0)
    ZEND_ARG_INFO(0, elements)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(predicate_arginfo, 0)
    ZEND_ARG_CALLABLE_INFO(0, predicate, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(selector_arginfo, 0)
    ZEND_ARG_CALLABLE_INFO(0, selector, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(other_arginfo, 0)
    ZEND_ARG_INFO(0, other)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(transform_arginfo, 0)
    ZEND_ARG_CALLABLE_INFO(0, transform, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(associate_by_arginfo, 0)
    ZEND_ARG_CALLABLE_INFO(0, key_selector, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(associate_by_to_arginfo, 0)
    ZEND_ARG_OBJ_INFO(0, destination, Collection, 0)
    ZEND_ARG_CALLABLE_INFO(0, key_selector, 0)
ZEND_END_ARG_INFO()

const zend_function_entry collections_collection_methods[] = {
    PHP_ME(Collection, __construct, NULL, ZEND_ACC_PRIVATE | ZEND_ACC_CTOR)
    PHP_ME(Collection, addAll, elements_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Collection, all, predicate_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Collection, any, predicate_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Collection, associate, transform_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Collection, associateBy, associate_by_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Collection, associateByTo, associate_by_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Collection, average, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Collection, containsAll, other_arginfo, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

const zend_function_entry collections_pair_methods[] = {
    PHP_FE_END
};