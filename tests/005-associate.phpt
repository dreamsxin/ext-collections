--TEST--
Test Collection::associate() and Collection::associateTo().
--FILE--
<?php
$array = ['foo' => 'bar', 34 => 5];
$collection = Collection::init($array)
    ->associate(function ($value, $key) {
        return new Pair($value, $key);
    });
if ($collection->toArray() != array_flip($array))
    echo 'Collection::associate() failed.', PHP_EOL;
$array1 = ['baz' => 1];
$collection1 = Collection::init($array1);
$array2 = $collection->associateTo($collection1,
    function ($value, $key) {
        return new Pair($key, $value);
    })->toArray();
if ($array2 != $array1 + array_flip($array) || $collection1->toArray() != $array2)
    echo 'Collection::associateTo() failed.', PHP_EOL;
?>
--EXPECT--
