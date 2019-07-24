# ext-collections

[![Travis-CI](https://travis-ci.org/CismonX/ext-collections.svg?branch=master)](https://travis-ci.org/CismonX/ext-collections)
[![MIT license](https://img.shields.io/badge/licence-MIT-blue.svg)](https://opensource.org/licenses/MIT)

## 1. Introduction

This PHP extension provides a set of useful functional-style operations on PHP arrays, which makes array manipulation simple and scalable.

Method names and functionalities are inspired by [Kotlin.Collections](https://kotlinlang.org/api/latest/jvm/stdlib/kotlin.collections/).

### 1.1 Notes

* Requires PHP 7.1 and above.
* Thread safety:
  * Distinct objects: **safe**.
  * Shared objects: **unsafe**.

## 2. Documentation

### 2.1 Functionalities

See [stubs](stubs/) directory for signature of all classes and methods of this extension, with PHPDoc. They can also serve as IDE helper.

### 2.2 PHP-style access

The `Collection` class implements `ArrayAccess` and `Countable` interface internally, you can treat an instance of `Collection` as an `ArrayObject`.

* The `isset()`, `unset()` keywords can be used on elements of `Collection`.
* Elements can be accessed via property and bracket expression.
* `empty()`, `count()` can be used on instance of `Collection`.
* Elements can be traversed via `foreach()` keyword.

### 2.3 Copy-on-write mechanism

Class `Collection` does not introduce new data structures internally. Instead, it only holds a pointer to a `zend_array`, and all its methods works directly on top of `zend_array`. Which means conversion between `Collection` and `array` does not involve copying, until write operation is performed on one of the duplicates.

```php
$foo = ['a', 'b'];              // arr0: refcount = 1
$bar = Collection::init($foo);  // arr0: refcount = 2, no copying of either `zend_array` or its elements
echo $bar->containsValue('a');  // arr0: refcount = 2, read operation, no copying
$bar->shuffle();                // arr0: refcount = 1, arr1: refcount = 1, write operation, `zend_array` is separated
$baz = $bar->toArray();         // arr0: refcount = 1, arr1: refcount = 2, no copying
```

### 2.4 Notes

* The `Collection::xxxTo()` methods will preserve the original key-value pairs of destination `Collection` when keys collide.
* Some methods of `Collection` involves comparing two of its elements, which accepts `$flags` as one of its arguments. When these methods are being invoked, make sure all elements are of the same type (numeric/string/others), otherwise you're likely to get a segfault.

## 3. Example

Here is a simple example for how to work with arrays gracefully using this extension.

```php
$employees = [
    ['name' => 'Alice', 'sex' => 'female', 'age' => 35],
    ['name' => 'Bob', 'sex' => 'male', 'age' => 29],
    ['name' => 'David', 'sex' => 'male', 'age' => 40],
    ['name' => 'Benjamin', 'sex' => 'male', 'age' => 32]
];
// Trying to get an array of names of male employees,
// sorted by the descending order of their age.
$names = Collection::init($employees)
    ->filter(function ($value) {
        return $value['sex'] == 'male';
    })
    ->sortedByDescending(function ($value) {
        return $value['age'];
    })
    ->map(function ($value) {
        return $value['name'];
    })
    ->toArray();
// You got $names == ['David', 'Benjamin', 'Bob'].
```
