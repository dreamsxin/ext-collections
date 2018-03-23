# ext-collections

[![Travis-CI](https://travis-ci.org/CismonX/ext-collections.svg?branch=master)](https://travis-ci.org/CismonX/ext-collections)
[![MIT license](https://img.shields.io/badge/licence-MIT-blue.svg)](https://opensource.org/licenses/MIT)

## 1. Introduction

This PHP extension provides a set of useful functional-style operations on PHP arrays, which makes array manipulation simple and scalable.

Method names and functionalities are inspired by [Kotlin.Collections](https://kotlinlang.org/api/latest/jvm/stdlib/kotlin.collections/).

It may take me a really long time to fully implement this extension. Feel free to contribute.

## 2. Documentation

See [stubs](stubs/) directory for signature of all classes and methods of this extension, with PHPDoc.

## 3. Example

Here is a simple example.

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
    ->filter(function ($it) {
        return $it->get('sex') == 'male';
    })
    ->sortedByDescending(function ($it) {
        return $it->get('age');
    })
    ->map(function ($it) {
        return $it->get('name');
    })
    ->toArray();
```

