<?php

/**
 * Wrapper for PHP array and provide functional-style operations.
 */
class Collection implements ArrayAccess, Countable
{
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
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return bool
     */
    function all($predicate) {}

    /**
     * Returns true if at least one element matches the given predicate.
     *
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return bool
     */
    function any($predicate) {}

    /**
     * Returns a collection containing key-value pairs provided by transform function applied
     * to elements of the given collection.
     *
     * @param callable $transform ($value) -> Pair
     * @return Collection
     */
    function associate($transform) {}

    /**
     * Populates and returns the destination collection with key-value pairs provided by transform
     * function applied to each element of the given collection.
     *
     * @param Collection $destination
     * @param callable $transform ($value) -> Pair
     * @return Collection
     */
    function associateTo($destination, $transform) {}

    /**
     * Returns a collection containing the elements from the given collection indexed by the key
     * returned from keySelector function applied to each element.
     *
     * @param callable $key_selector ($value) -> $key
     * @return Collection
     */
    function associateBy($key_selector) {}

    /**
     * Populates and returns the destination collection with key-value pairs, where key is
     * provided by the keySelector function applied to each element of the given collection
     * and value is the element itself.
     *
     * @param Collection $destination
     * @param callable $key_selector ($value) -> $key
     * @return Collection
     */
    function associateByTo($destination, $key_selector) {}

    /**
     * Returns an average value of elements in the collection.
     *
     * @return double
     */
    function average() {}

    /**
     * Checks if all elements in the specified collection are contained in this collection.
     *
     * @param array|Collection $other
     * @return bool
     */
    function containsAll($other) {}

    /**
     * Checks if the array contains the given key.
     *
     * @param int|string $key
     * @return bool
     */
    function containsKey($key) {}

    /**
     * Check if the array maps one or more keys to the specified value.
     *
     * @param mixed $element
     * @return bool
     */
    function containsValue($element) {}

    /**
     * Returns new array which is a copy of the original array.
     *
     * @param array|Collection $elements
     * @param int $new_size[optional]
     * @return Collection
     */
    function copyOf($elements, $new_size) {}

    /**
     * Returns new array which is a copy of range of original array.
     *
     * @param int $from_index
     * @param int $to_index
     * @return Collection
     */
    function copyOfRange($from_index, $to_index) {}

    /**
     * Returns the number of elements in this array.
     *
     * @return int
     */
    function count() {}

    /**
     * Returns a collection containing only distinct elements from the given array.
     *
     * @return Collection
     */
    function distinct() {}

    /**
     * Returns a collection containing only elements from the given array having distinct keys
     * returned by the given selector function.
     *
     * @param callable $selector ($value, $key, $index) -> $key
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
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return Collection
     */
    function dropLastWhile($predicate) {}

    /**
     * Returns a collection containing all elements except first elements that satisfy
     * the given predicate.
     *
     * @param $predicate $predicate ($value, $key, $index) -> bool
     * @return Collection
     */
    function dropWhile($predicate) {}

    /**
     * Fills original array with the provided value.
     *
     * @param mixed $element
     * @param int $from_index[optional]
     * @param int $to_index[optional]
     * @return Collection
     */
    function fill($element, $from_index, $to_index) {}

    /**
     * Returns a collection containing only elements matching the given predicate.
     *
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return Collection
     */
    function filter($predicate) {}

    /**
     * Returns a collection containing only elements not matching the given predicate.
     *
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return Collection
     */
    function filterNot($predicate) {}

    /**
     * Appends all elements not matching the given predicate to the given destination.
     *
     * @param Collection $destination
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return Collection
     */
    function filterNotTo($destination, $predicate) {}

    /**
     * Appends all elements matching the given predicate to the given destination.
     *
     * @param Collection $destination
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return Collection
     */
    function filterTo($destination, $predicate) {}

    /**
     * Returns the first element matching the given predicate, or null if no such
     * element was found.
     *
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return mixed
     */
    function find($predicate) {}

    /**
     * Returns the last element matching the given predicate, or null if no such
     * element was found.
     *
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return mixed
     */
    function findLast($predicate) {}

    /**
     * Returns the first element matching the given predicate.
     *
     * @param callable $predicate[optional] ($value, $key, $index) -> bool
     * @return mixed
     */
    function first($predicate) {}

    /**
     * Returns a collection of all elements yielded from results of transform function
     * being invoked on each element of original collection.
     *
     * @param callable $transform ($value, $key, $index) -> array|Collection
     * @return Collection
     */
    function flatMap($transform) {}

    /**
     * Appends all elements yielded from results of transform function being invoked on each element of
     * original collection, to the given destination.
     *
     * @param Collection $destination
     * @param callable $transform ($value, $key, $index) -> array|Collection
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
     * @param callable $action ($value, $key, $index) -> void
     * @return void
     */
    function forEach($action) {}

    /**
     * Returns an element at the given key or the result of calling the `default` function
     * if the index is out of bounds of this array.
     *
     * @param int|string $key
     * @param callable $default[optional] ($key) -> $value
     * @return Collection
     */
    function get($key, $default) {}

    /**
     * Groups values returned by the value_transform function applied to each element of the
     * original array by the key returned by the given key_selector function applied to the element
     * and returns a collection where each group key is associated with a list of corresponding values.
     *
     * @param callable $key_selector ($value, $key, $index) -> $new_key
     * @param callable $value_transform[optional] ($value) -> $new_value
     * @return Collection
     */
    function groupBy($key_selector, $value_transform) {}

    /**
     * Groups values returned by the value_transform function applied to each element of the original
     * collection by the key returned by the given key_selector function applied to the element and
     * puts to the destination collection each group key associated with a list of corresponding values.
     *
     * @param Collection $destination
     * @param callable $key_selector ($value, $key, $index) -> $new_key
     * @param callable $value_transform[optional] ($value) -> $new_value
     * @return Collection
     */
    function groupByTo($destination, $key_selector, $value_transform) {}

    /**
     * Returns first key of element, or null if the array does not contain element.
     *
     * @param mixed $element
     * @return int|string|null
     */
    function indexOf($element) {}

    /**
     * Returns key of the first element matching the given predicate, or null if the array does
     * not contain such element.
     *
     * @param callable $predicate ($value) -> bool
     * @return int|string|null
     */
    function indexOfFirst($predicate) {}

    /**
     * Returns key of the last element matching the given predicate, or null if the array does
     * not contain such element.
     *
     * @param callable $predicate ($value) -> bool
     * @return int|string|null
     */
    function indexOfLast($predicate) {}

    /**
     * Initialize collection with given data.
     *
     * @param array|Collection $elements[$optional]
     * @return Collection
     */
    static function init($elements) {}

    /**
     * Returns a collection containing all elements that are contained by both this collection and the
     * specified collection.
     *
     * @param array|Collection $other
     */
    function intersect($other) {}

    /**
     * Returns true if the array is empty.
     *
     * @return bool
     */
    function isEmpty() {}

    /**
     * Returns true if the array is not empty.
     */
    function isNotEmpty() {}

    /**
     * Return a collection of all keys in the collection.
     *
     * @return Collection
     */
    function keys() {}

    /**
     * Returns the last element matching the given predicate.
     *
     * @param callable $predicate[optional] ($value, $key, $index) -> bool
     * @return mixed
     */
    function last($predicate) {}

    /**
     * Returns last key of element, or null if the array does not contain element.
     *
     * @param mixed $element
     * @return int|string|null
     */
    function lastIndexOf($element) {}

    /**
     * Returns a collection containing the results of applying the given transform function
     * to each element in the original collection.
     *
     * @param callable $transform ($value, $key, $index) -> Pair
     * @return Collection
     */
    function map($transform) {}

    /**
     * Returns a new collection with entries having the keys obtained by applying the transform function
     * to each keys and values of this collection.
     *
     * @param callable $transform ($value, $key, $index) -> $new_key
     * @return Collection
     */
    function mapKeys($transform) {}

    /**
     * Populates the given destination collection with entries having the keys obtained by applying the
     * transform function to each keys and values of this collections.
     *
     * @param Collection $destination
     * @param callable $transform ($value, $key, $index) -> $new_key
     * @return Collection
     */
    function mapKeysTo($destination, $transform) {}

    /**
     * Applies the given transform function to each element of the original collection and appends the
     * results to the given destination.
     *
     * @param Collection $destination
     * @param callable $transform ($value, $key, $index) -> Pair
     * @return Collection
     */
    function mapTo($destination, $transform) {}

    /**
     * Returns a new collection with entries having the keys of this collection and the values obtained
     * by applying the transform function to each entry in this collection.
     *
     * @param callable $transform ($value, $key, $index) -> $new_value
     * @return Collection
     */
    function mapValues($transform) {}

    /**
     * Populates the given destination collection with entries having the keys of this collection and
     * the values obtained by applying the transform function to each entry in this collection.
     *
     * @param Collection $destination
     * @param callable $transform ($value, $key, $index) -> $new_value
     * @return Collection
     */
    function mapValuesTo($destination, $transform) {}

    /**
     * Returns the largest element or null if there are no elements.
     *
     * @return mixed
     */
    function max() {}

    /**
     * Returns the first element yielding the largest value of the given function or null if
     * there are no elements.
     *
     * @param callable $selector ($value, $key, $index) -> $new_value
     * @return mixed
     */
    function maxBy($selector) {}

    /**
     * Returns the first element having the largest value according to the provided comparator or
     * null if there are no elements.
     *
     * @param callable $comparator ($value_1, $value_2, $key_1, $key_2) -> int
     * @return mixed
     */
    function maxWith($comparator) {}

    /**
     * Returns the largest element or null if there are no elements.
     *
     * @return mixed
     */
    function min() {}

    /**
     * Returns the first element yielding the smallest value of the given function or null if
     * there are no elements.
     *
     * @param callable $selector ($value, $key, $index) -> $new_value
     * @return mixed
     */
    function minBy($selector) {}

    /**
     * Returns the first element having the smallest value according to the provided comparator or
     * null if there are no elements.
     *
     * @param callable $comparator ($value_1, $value_2, $key_1, $key_2) -> int
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
     * Removes all key-value pairs contained in the given collection from this collection.
     *
     * @param array|Collection $elements
     * @return void
     */
    function minusAssign($elements) {}

    /**
     * Returns a collection containing all entries of the original collection except those entries
     * the keys of which are contained in the given keys collection.
     *
     * @param array|Collection $keys
     * @return Collection
     */
    function minusKeys($keys) {}

    /**
     * Removes all entries the keys of which are contained in the given keys collection from this
     * collection.
     *
     * @param array|Collection $keys
     * @return void
     */
    function minusKeysAssign($keys) {}

    /**
     * Returns a list containing all elements of the original collection except the elements contained
     * in the given elements collection.
     *
     * @param array|Collection $elements
     * @return Collection
     */
    function minusValues($elements) {}

    /**
     * Removes all elements contained in the given elements collection from this collection.
     *
     * @param array|Collection $elements
     * @return void
     */
    function minusValuesAssign($elements) {}

    /**
     * Returns true if no elements match the given predicate.
     *
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return bool
     */
    function none($predicate) {}

    /**
     * @internal
     */
    function offsetUnset($key) {}

    /**
     * @internal
     */
    function offsetSet($key, $value) {}

    /**
     * @internal
     */
    function offsetGet($key) {}

    /**
     * @internal
     */
    function offsetExists($key) {}

    /**
     * Performs the given action on each element and returns the collection itself afterwards.
     *
     * @param callable $action ($value, $key, $index) -> void
     * @return Collection
     */
    function onEach($action) {}

    /**
     * Returns this collection if it's not null and the empty collection otherwise.
     *
     * @return Collection
     */
    function orEmpty() {}

    /**
     * Splits the original collection into pair of collections, where first collection contains
     * elements for which predicate yielded true, while second collection contains elements for
     * which predicate yielded false.
     *
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return Pair
     */
    function partition($predicate) {}

    /**
     * Creates a new collection by replacing or adding entries to this collection from a given
     * collection of key-value pairs.
     *
     * @param array|Collection $elements
     * @return Collection
     */
    function plus($elements) {}

    /**
     * Appends or replaces all pairs from the given collection of pairs in this collection.
     *
     * @param array|Collection $elements
     * @return void
     */
    function plusAssign($elements) {}

    /**
     * Returns a collection containing all elements of the original collection and then all elements
     * of the given elements collection.
     *
     * @param array|Collection $elements
     * @return Collection
     */
    function plusValues($elements) {}

    /**
     * Adds all elements of the given elements collection to this collection.
     *
     * @param array|Collection $elements
     * @return void
     */
    function plusValuesAssign($elements) {}

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
     * @param $operation ($acc, $value, $key) -> $new_acc
     * @return mixed
     */
    function reduce($operation) {}

    /**
     * Accumulates value starting with the last element and applying operation from right to left to
     * each element and current accumulator value.
     *
     * @param $operation ($acc, $value, $key) -> $new_acc
     * @return mixed
     */
    function reduceRight($operation) {}

    /**
     * Removes the entry for the specified key only if it is currently mapped to the specified value.
     *
     * @param int|string $key
     * @param mixed $value[optional]
     */
    function remove($key, $value) {}

    /**
     * Removes all elements from this collection that match the given predicate.
     *
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return bool
     */
    function removeAll($predicate) {}

    /**
     * Retains only elements of this collection that match the given predicate.
     *
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return bool
     */
    function retainAll($predicate) {}

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
     * Randomly shuffles elements in this collection.
     *
     * @return Collection
     */
    function shuffle() {}

    /**
     * Returns the single element matching the given predicate, or null if there is no or more than
     * one matching element.
     *
     * @param callable $predicate ($value, $key, $index) -> bool
     * @return mixed
     */
    function single($predicate) {}

    /**
     * Returns a list containing elements at specified keys.
     *
     * @param array|Collection $keys
     */
    function slice($keys) {}

    /**
     * Returns a collection containing elements at indices in the specified indices range.
     *
     * @param int $from_index
     * @param int $to_index[optional]
     */
    function sliceRange($from_index, $to_index) {}

    /**
     * Sorts the collection in-place according to the natural order of its elements.
     *
     * @return void
     */
    function sort() {}

    /**
     * Sorts elements in the collection in-place according to natural sort order of the value returned
     * by specified selector function.
     *
     * @param callable $selector ($value, $key, $index) -> $new_value
     * @return void
     */
    function sortBy($selector) {}

    /**
     * Sorts elements in the collection in-place descending according to natural sort order of the
     * value returned by specified selector function.
     *
     * @param callable $selector ($value, $key, $index) -> $new_value
     * @return void
     */
    function sortByDescending($selector) {}

    /**
     * Sorts elements in the collection in-place descending according to their natural sort order.
     *
     * @return void
     */
    function sortDescending() {}

    /**
     * Sorts elements in the collection in-place according to the order specified with comparator.
     *
     * @param callable $comparator ($value_1, $value_2, $key_1, $key_2) -> int
     * @return void
     */
    function sortWith($comparator) {}

    /**
     * Returns a collection of all elements sorted according to their natural sort order.
     *
     * @return Collection
     */
    function sorted() {}

    /**
     * Returns a collection of all elements sorted according to natural sort order of the
     * value returned by specified selector function.
     *
     * @param callable $selector ($value, $key, $index) -> $new_value
     * @return Collection
     */
    function sortedBy($selector) {}

    /**
     * Returns a collection of all elements sorted descending according to natural sort order
     * of the value returned by specified selector function.
     *
     * @param callable $selector ($value, $key, $index) -> $new_value
     * @return Collection
     */
    function sortedByDescending($selector) {}

    /**
     * Returns a collection of all elements sorted descending according to their natural sort order.
     *
     * @param callable $selector ($value, $key, $index) -> $new_value
     * @return Collection
     */
    function sortedDescending($selector) {}

    /**
     * Returns a collection of all elements sorted according to the specified comparator.
     *
     * @param callable $comparator ($value_1, $value_2, $key_1, $key_2) -> int
     * @return Collection
     */
    function sortedWith($comparator) {}

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
     * @param $predicate ($value, $key, $index) -> bool
     * @return Collection
     */
    function takeLastWhile($predicate) {}

    /**
     * Returns a collection containing first elements satisfying the given predicate.
     *
     * @param $predicate ($value, $key, $index) -> bool
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