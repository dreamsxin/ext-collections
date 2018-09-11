<?php

/**
 * Wrapper for PHP array and provide functional-style operations.
 */
class Collection implements ArrayAccess, Countable
{
    /**
     * Compare strings in alphabetical order, except that multi-digit numbers are ordered as a
     * single character. Only used when comparing strings.
     */
    const COMPARE_NATRUAL = 1;

    /**
     * Elements will be campared in a case-insensitive manner. Only used when comparing strings.
     */
    const FOLD_CASE = 2;

    /**
     * Private constructor.
     * The Collection class should be initialized with static method Collection::init($data).
     */
    private function __construct() {}

    /**
     * Adds all elements of the given elements collection to this Collection.
     *
     * @param array|Collection $elements
     * @return void
     */
    function addAll($elements) {}

    /**
     * Returns true if all elements match the given predicate.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return bool
     */
    function all($predicate) {}

    /**
     * Returns true if at least one element matches the given predicate.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return bool
     */
    function any($predicate) {}

    /**
     * Returns a collection containing key-value pairs provided by transform function applied
     * to elements of the given collection.
     *
     * @param callable $transform ($value, $key) -> Pair
     * @return Collection
     */
    function associate($transform) {}

    /**
     * Populates and returns the destination collection with key-value pairs provided by transform
     * function applied to each element of the given collection.
     *
     * @param Collection $destination
     * @param callable $transform ($value, $key) -> Pair
     * @return Collection
     */
    function associateTo($destination, $transform) {}

    /**
     * Returns a collection containing the elements from the given collection indexed by the key
     * returned from keySelector function applied to each element.
     *
     * @param callable $key_selector ($value, $key) -> $new_key
     * @return Collection
     */
    function associateBy($key_selector) {}

    /**
     * Populates and returns the destination collection with key-value pairs, where key is
     * provided by the keySelector function applied to each element of the given collection
     * and value is the element itself.
     *
     * @param Collection $destination
     * @param callable $key_selector ($value, $key) -> $new_key
     * @return Collection
     */
    function associateByTo($destination, $key_selector) {}

    /**
     * Returns an average value of elements in the collection.
     *
     * @return double|null
     */
    function average() {}

    /**
     * Searches the array or the range of the array for the provided element using the
     * binary search algorithm.
     * 
     * The array is expected to be packed and sorted into ascending order, otherwise the
     * result is undefined.
     * 
     * @param mixed $element
     * @param int $flags[optional]
     * @param int $from_index[optional]
     * @param int $num_elements[optional]
     * @return int|null
     */
    function binarySearch($element, $flags, $from_index, $num_elements) {}

    /**
     * Searches the array or the range of the array for the provided element using the
     * binary search algorithm, where the element equals to the one returned by the
     * corresponding selector function.
     * 
     * The array is expected to be packed and sorted into ascending order, otherwise the
     * result is undefined.
     * 
     * @param mixed $element
     * @param callable $selector ($value, $key) -> $new_value
     * @param int $flags[optional]
     * @param int $from_index[optional]
     * @param int $num_elements[optional]
     * @return int|null
     */
    function binarySearchBy($element, $selector, $flags, $from_index, $num_elements) {}

    /**
     * Splits this collection into a collection of arrays each not exceeding the given size.
     * 
     * If the transform function is provided, apply it on each array.
     * 
     * @param int $size
     * @param callable $transform[optional] ($value, $key) -> $new_value
     * @return Collection
     */
    function chunked($size, $transform) {}

    /**
     * Checks if all key-value pairs in the specified collection are contained in this collection.
     *
     * @param array|Collection $other
     * @return bool
     */
    function containsAll($other) {}

    /**
     * Checks if all keys in the specified collection are contained in this collection.
     *
     * @param array|Collection $other
     * @return bool
     */
    function containsAllKeys($other) {}

        /**
     * Checks if all values in the specified collection are contained in this collection.
     *
     * @param array|Collection $other
     * @return bool
     */
    function containsAllValues($other) {}

    /**
     * Checks if the collection contains the given key.
     *
     * @param int|string $key
     * @return bool
     */
    function containsKey($key) {}

    /**
     * Check if the collection maps one or more keys to the specified value.
     *
     * @param mixed $element
     * @return bool
     */
    function containsValue($element) {}

    /**
     * Returns new collection which is a copy of the original collection.
     *
     * @param int $new_size[optional]
     * @return Collection
     */
    function copyOf($new_size) {}

    /**
     * Returns new collection which is a copy of range of original collection.
     *
     * @param int $from_index
     * @param int $num_elements
     * @return Collection
     */
    function copyOfRange($from_index, $num_elements) {}

    /**
     * Returns the number of elements in this collection.
     *
     * @return int
     */
    function count() {}

    /**
     * Returns a collection containing only distinct elements from the given collection.
     * 
     * If the collection is an ordinary array, the element order in the returned collection is
     * guaranteed to be preserved, and integer index will be reordered.
     *
     * @return Collection
     */
    function distinct() {}

    /**
     * Returns a collection containing only elements from the given collection having distinct
     * values returned by the given selector function.
     * 
     * If the collection is an ordinary array, the element order in the returned collection is
     * guaranteed to be preserved, and integer index will be reordered.
     *
     * @param callable $selector ($value, $key) -> $new_value
     * @return Collection
     */
    function distinctBy($selector) {}

    /**
     * Returns a list containing all elements except first n elements.
     *
     * @param int $n
     * @return Collection
     */
    function drop($n) {}

    /**
     * Returns a collection containing all elements except last n elements.
     *
     * @param int $n
     * @return Collection
     */
    function dropLast($n) {}

    /**
     * Returns a collection containing all elements except last elements that satisfy
     * the given predicate.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return Collection
     */
    function dropLastWhile($predicate) {}

    /**
     * Returns a collection containing all elements except first elements that satisfy
     * the given predicate.
     *
     * @param $predicate $predicate ($value, $key) -> bool
     * @return Collection
     */
    function dropWhile($predicate) {}

    /**
     * Fills original collection with the provided value.
     *
     * @param mixed $element
     * @param int $from_index[optional]
     * @param int $num_elements[optional]
     * @return void
     */
    function fill($element, $from_index, $num_elements) {}

    /**
     * Returns a collection containing only elements matching the given predicate.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return Collection
     */
    function filter($predicate) {}

    /**
     * Returns a collection containing only elements not matching the given predicate.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return Collection
     */
    function filterNot($predicate) {}

    /**
     * Appends all elements not matching the given predicate to the given destination.
     *
     * @param Collection $destination
     * @param callable $predicate ($value, $key) -> bool
     * @return Collection
     */
    function filterNotTo($destination, $predicate) {}

    /**
     * Appends all elements matching the given predicate to the given destination.
     *
     * @param Collection $destination
     * @param callable $predicate ($value, $key) -> bool
     * @return Collection
     */
    function filterTo($destination, $predicate) {}

    /**
     * Returns the first element matching the given predicate, or null if no such
     * element was found.
     *
     * @param callable $predicate[optional] ($value, $key) -> bool
     * @return mixed
     */
    function first($predicate) {}

    /**
     * Returns a collection of all elements yielded from results of transform function
     * being invoked on each element of original collection.
     *
     * @param callable $transform ($value, $key) -> array|Collection
     * @return Collection
     */
    function flatMap($transform) {}

    /**
     * Appends all elements yielded from results of transform function being invoked on each element of
     * original collection, to the given destination.
     *
     * @param Collection $destination
     * @param callable $transform ($value, $key) -> array|Collection
     * @return Collection
     */
    function flatMapTo($destination, $transform) {}

    /**
     * Returns a single list of all elements from all collections in the given collection.
     *
     * @return Collection
     */
    function flatten() {}

    /**
     * Accumulates value starting with initial value and applying operation from left to
     * right to current accumulator value and each element.
     *
     * @param mixed $initial
     * @param callable $operation ($acc, $value, $key) -> $new_acc
     * @return mixed
     */
    function fold($initial, $operation) {}

    /**
     * Accumulates value starting with initial value and applying operation from right to
     * left to each element and current accumulator value.
     *
     * @param mixed $initial
     * @param callable $operation ($acc, $value, $key) -> $new_acc
     * @return mixed
     */
    function foldRight($initial, $operation) {}

    /**
     * Performs the given action on each element.
     *
     * @param callable $action ($value, $key) -> void
     * @return void
     */
    function forEach($action) {}

    /**
     * Returns an element at the given key or the result of calling the `default` function
     * if the index is out of bounds of this collection.
     *
     * @param int|string $key
     * @param callable $default[optional] ($key) -> $value
     * @return Collection
     */
    function get($key, $default) {}

    /**
     * Groups values by the key returned by the given transform function applied to the element
     * and returns a collection where each group key is associated with a list of corresponding values.
     * 
     * Key should be either string or integer. If the transform function returns a Pair, the
     * original value will be mapped to the new value.
     *
     * @param callable $transform ($value, $key) -> $new_key | Pair($new_key, $new_value)
     * @return Collection
     */
    function groupBy($transform) {}

    /**
     * Groups values by the key returned by the given transform function applied to the element and
     * puts to the destination collection each group key associated with a list of corresponding values.
     * 
     * Key should be either string or integer. If the transform function returns a Pair, the
     * original value will be mapped to the new value.
     *
     * @param Collection $destination
     * @param callable $transform ($value, $key) -> $new_key | Pair($new_key, $new_value)
     * @return Collection
     */
    function groupByTo($destination, $transform) {}

    /**
     * Returns first key of element, or null if the collection does not contain element.
     *
     * @param mixed $element
     * @return int|string|null
     */
    function indexOf($element) {}

    /**
     * Returns key of the first element matching the given predicate, or null if the collection
     * does not contain such element.
     *
     * @param callable $predicate[optional] ($value) -> bool
     * @return int|string|null
     */
    function indexOfFirst($predicate) {}

    /**
     * Returns key of the last element matching the given predicate, or null if the collection
     * does not contain such element.
     *
     * @param callable $predicate[optional] ($value) -> bool
     * @return int|string|null
     */
    function indexOfLast($predicate) {}

    /**
     * Initialize collection with given data.
     *
     * @param array|Collection $elements[optional]
     * @return Collection
     */
    static function init($elements) {}

    /**
     * Returns a collection containing all key-value pairs that are contained by both this collection
     * and the specified collection.
     *
     * @param array|Collection $other
     * @return Collection
     */
    function intersect($other) {}

    /**
     * Returns a collection containing all elements with keys contained by both this collection
     * and the specified collection. Values are that of the original collection.
     *
     * @param array|Collection $other
     * @return Collection
     */
    function intersectKeys($other) {}

    /**
     * Returns a collection containing all elements with values contained by both this collection
     * and the specified collection. Keys are that of the original collection.
     * 
     * Duplicate values will be removed.
     *
     * @param array|Collection $other
     * @return Collection
     */
    function intersectValues($other) {}

    /**
     * Returns true if the collection is empty.
     *
     * @return bool
     */
    function isEmpty() {}

    /**
     * Check whether the collection wraps a packed hashtable (regular array).
     *
     * @return bool
     */
    function isPacked() {}

    /**
     * Return a collection of all keys in the collection.
     *
     * @return Collection
     */
    function keys() {}

    /**
     * Returns the last element matching the given predicate, or null if no such
     * element was found.
     *
     * @param callable $predicate[optional] ($value, $key) -> bool
     * @return mixed
     */
    function last($predicate) {}

    /**
     * Returns last key of element, or null if the collection does not contain element.
     *
     * @param mixed $element
     * @return int|string|null
     */
    function lastIndexOf($element) {}

    /**
     * Returns a collection containing an array of values obtained by applying the transform function
     * to each element of the original collection.
     *
     * @param callable $transform ($value, $key) -> $new_value
     * @return Collection
     */
    function map($transform) {}

    /**
     * Populates the given destination collection with an array of values obtained by applying the
     * transform function to each element of the original collection.
     *
     * @param Collection $destination
     * @param callable $transform ($value, $key) -> $new_value
     * @return Collection
     */
    function mapTo($destination, $transform) {}

    /**
     * Returns the largest element or null if there are no elements. The collection should contain
     * only one type of numeric elements(int/double).
     *
     * @param int $flags[optional]
     * @return mixed
     */
    function max($flags) {}

    /**
     * Returns the first element yielding the largest value of the given function or null if
     * there are no elements.
     *
     * @param callable $selector ($value, $key) -> $new_value
     * @param int $flags[optional]
     * @return mixed
     */
    function maxBy($selector, $flags) {}

    /**
     * Returns the first element having the largest value according to the provided comparator or
     * null if there are no elements.
     *
     * @param callable $comparator (Pair($key, $value), Pair($key, $value)) -> int
     * @return mixed
     */
    function maxWith($comparator) {}

    /**
     * Returns the largest element or null if there are no elements. The collection should contain
     * only one type of numeric elements(int/double).
     *
     * @param int $flags[optional]
     * @return mixed
     */
    function min($flags) {}

    /**
     * Returns the first element yielding the smallest value of the given function or null if
     * there are no elements.
     *
     * @param callable $selector ($value, $key) -> $new_value
     * @param int $flags[optional]
     * @return mixed
     */
    function minBy($selector, $flags) {}

    /**
     * Returns the first element having the smallest value according to the provided comparator or
     * null if there are no elements.
     *
     * @param callable $comparator (Pair($key, $value), Pair($key, $value)) -> int
     * @return mixed
     */
    function minWith($comparator) {}

    /**
     * Returns a list containing all key-value pairs of the original collection except the ones contained
     * in the given elements collection.
     *
     * @param array|Collection $elements
     * @return Collection
     */
    function minus($elements) {}

    /**
     * Returns true if no elements match the given predicate.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return bool
     */
    function none($predicate) {}

    /**
     * This method is implemented in handlers.unset_dimension.
     * There's no method with name 'offsetUnset'.
     *
     * @internal
     */
    function offsetUnset($key) {}

    /**
     * This method is implemented in handlers.write_dimension.
     * There's no method with name 'offsetSet'.
     *
     * @internal
     */
    function offsetSet($key, $value) {}

    /**
     * This method is implemented in handlers.read_dimension.
     * There's no method with name 'offsetGet'.
     *
     * @internal
     */
    function offsetGet($key) {}

    /**
     * This method is implemented in handlers.has_dimension.
     * There's no method with name 'offsetExists'.
     *
     * @internal
     */
    function offsetExists($key) {}

    /**
     * Performs the given action on each element and returns the collection itself afterwards.
     *
     * @param callable $action ($value, $key) -> void
     * @return Collection
     */
    function onEach($action) {}

    /**
     * Splits the original collection into pair of collections, where first collection contains
     * elements for which predicate yielded true, while second collection contains elements for
     * which predicate yielded false.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return Pair
     */
    function partition($predicate) {}

    /**
     * Creates a new collection by replacing or elements to this collection from a given collection.
     *
     * @param array|Collection $elements
     * @return Collection
     */
    function plus($elements) {}

    /**
     * Puts all the elements of the given collection into this collection.
     *
     * @param array|Collection $elements
     * @return void
     */
    function putAll($elements) {}

    /**
     * Accumulates value starting with the first element and applying operation from left to right to
     * current accumulator value and each element.
     *
     * @param callable $operation ($acc, $value, $key) -> $new_acc
     * @return mixed
     */
    function reduce($operation) {}

    /**
     * Accumulates value starting with the last element and applying operation from right to left to
     * each element and current accumulator value.
     *
     * @param callable $operation ($acc, $value, $key) -> $new_acc
     * @return mixed
     */
    function reduceRight($operation) {}

    /**
     * Removes the entry for the specified key only if it is currently mapped to the specified value.
     *
     * @param int|string $key
     * @param mixed $value[optional]
     * @return bool
     */
    function remove($key, $value) {}

    /**
     * Removes elements of this collection whose values exists in the given collection.
     *
     * @param array|Collection $elements[optional]
     * @return void
     */
    function removeAll($elements) {}

    /**
     * Removes all elements from this collection that match the given predicate.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return void
     */
    function removeWhile($predicate) {}

    /**
     * Retains elements of this collection whose values exists in the given collection.
     *
     * @param array|Collection $elements
     * @return void
     */
    function retainAll($elements) {}

    /**
     * Retains all elements from this collection that match the given predicate.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return void
     */
    function retainWhile($predicate) {}

    /**
     * Reverses elements in the collection in-place.
     *
     * @return void
     */
    function reverse() {}

    /**
     * Returns a collection with elements in reversed order.
     *
     * @return Collection
     */
    function reversed() {}

    /**
     * Add a key-value pair to the array. If the key exists, update it.
     *
     * @param int|string $key
     * @param mixed $value
     * @return void
     */
    function set($key, $value) {}

    /**
     * Randomly shuffles elements in this collection. Keys will be discarded.
     *
     * @return void
     */
    function shuffle() {}

    /**
     * Returns a shuffled copy of this collection. Keys will be discarded.
     *
     * @return Collection
     */
    function shuffled() {}

    /**
     * Returns the single element matching the given predicate, or null if there is no or more than
     * one matching element.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return mixed
     */
    function single($predicate) {}

    /**
     * Returns a list containing elements at specified keys.
     *
     * @param array|Collection $keys
     * @return Collection
     */
    function slice($keys) {}

    /**
     * Sorts the collection in-place according to the specified order of its elements.
     *
     * @param int $flags[optional]
     * @return void
     */
    function sort($flags) {}

    /**
     * Sorts elements in the collection in-place according to the specified order of the value returned
     * by specified selector function.
     *
     * @param callable $selector ($value, $key) -> $new_value
     * @param int $flags[optional]
     * @return void
     */
    function sortBy($selector, $flags) {}

    /**
     * Sorts elements in the collection in-place descending according to the specified order of the
     * value returned by specified selector function.
     *
     * @param callable $selector ($value, $key) -> $new_value
     * @param int $flags[optional]
     * @return void
     */
    function sortByDescending($selector, $flags) {}

    /**
     * Sorts elements in the collection in-place descending according to the specified order.
     *
     * @param int $flags[optional]
     * @return void
     */
    function sortDescending($flags) {}

    /**
     * Sorts elements in the collection in-place according to the order specified with comparator.
     *
     * @param callable $comparator (Pair($key, $value), Pair($key, $value)) -> int
     * @return void
     */
    function sortWith($comparator) {}

    /**
     * Returns a collection of all elements sorted according to the specified order.
     *
     * @param int $flags[optional]
     * @return Collection
     */
    function sorted($flags) {}

    /**
     * Returns a collection of all elements sorted according to the specified order of the
     * value returned by specified selector function.
     *
     * @param callable $selector ($value, $key) -> $new_value
     * @param int $flags[optional]
     * @return Collection
     */
    function sortedBy($selector, $flags) {}

    /**
     * Returns a collection of all elements sorted descending according to the specified order
     * of the value returned by specified selector function.
     *
     * @param callable $selector ($value, $key) -> $new_value
     * @param int $flags[optional]
     * @return Collection
     */
    function sortedByDescending($selector, $flags) {}

    /**
     * Returns a collection of all elements sorted descending according to the specified order.
     *
     * @param callable $selector ($value, $key) -> $new_value
     * @param int $flags[optional]
     * @return Collection
     */
    function sortedDescending($selector, $flags) {}

    /**
     * Returns a collection of all elements sorted according to the specified comparator.
     *
     * @param callable $comparator (Pair($key, $value), Pair($key, $value)) -> int
     * @return Collection
     */
    function sortedWith($comparator) {}

    /**
     * Returns a collection containing all distinct elements that are contained by this
     * collection and not contained by the specified collection.
     *
     * @param array|Collection $other
     * @return Collection
     */
    function subtract($other) {}

    /**
     * Returns the sum of all elements in the collection.
     * 
     * All elements should be of the same type, int or double. Otherwise result is undefined.
     * 
     * @return int|double|null
     */
    function sum() {}

    /**
     * Returns the sum of all values produced by selector function applied to each element
     * in the collection.
     * 
     * All return values of the selector function should be of the same type, int or double.
     * Otherwise result is undefined.
     * 
     * @param callable $selector ($value, $key) -> int|double
     * @return int|double|null
     */
    function sumBy($selector) {}

    /**
     * Returns a collection containing first n elements.
     *
     * @param int $n
     * @return Collection
     */
    function take($n) {}

    /**
     * Returns a list containing last n elements.
     *
     * @param int $n
     * @return Collection
     */
    function takeLast($n) {}

    /**
     * Returns a collection containing last elements satisfying the given predicate.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return Collection
     */
    function takeLastWhile($predicate) {}

    /**
     * Returns a collection containing first elements satisfying the given predicate.
     *
     * @param callable $predicate ($value, $key) -> bool
     * @return Collection
     */
    function takeWhile($predicate) {}

    /**
     * Returns the array wrapped by this collection.
     *
     * @return array
     */
    function toArray() {}

    /**
     * Appends all elements to the given destination collection.
     *
     * @param Collection $destination
     * @return Collection
     */
    function toCollection($destination) {}

    /**
     * Convert collection to a collection of pairs with keys being first and values being second.
     *
     * @return Collection
     */
    function toPairs() {}

    /**
     * Returns a collection containing all distinct elements from both collections.
     *
     * @param array|Collection $other
     */
    function union($other) {}

    /**
     * Return a collection of all values of the collection.
     *
     * @return Collection
     */
    function values() {}
}
