--TEST--
Test Collection::take() and Collection::takeLast().
--FILE--
<?php
$array = ['a', 'b', 'c', 'd', 'e'];
$collection = Collection::init($array);
if ($collection->take(0)->toArray() != [] || $collection->take(6)->toArray() != $array ||
    $collection->take(3)->toArray() != array_slice($array, 0, 3))
    echo 'Collection::take() failed.', PHP_EOL;
if ($collection->takeLast(0)->toArray() != [] || $collection->takeLast(10)->toArray() != $array ||
    $collection->takeLast(4)->toArray() != array_slice($array, -4, 4))
    echo 'Collection::takeLast() failed.', PHP_EOL;
?>
--EXPECT--
