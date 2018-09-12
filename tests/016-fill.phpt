--TEST--
Test Collection::fill().
--FILE--
<?php
$array = ['foo', 'bar' => 'baz', 1, 2, 3];
$collection = Collection::init($array);
$collection->fill('t', 1, 4);
$array1 = ['foo', 'bar' => 't', 't', 't', 3];
$collection1 = Collection::init($array);
$collection1->fill(0);
$array2 = [0, 'bar' => 0, 0, 0, 0];
if ($collection->toArray() != $array1 || $collection1->toArray() != $array2) {
    echo 'Collection::fill() failed.', PHP_EOL;
}
?>
--EXPECT--
