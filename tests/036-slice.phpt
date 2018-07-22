--TEST--
Test Collection::slice().
--FILE--
<?php
$array = [1, 2, 3, 4, 5, 6];
$collection = Collection::init($array)->slice([2, -1, 3, 6]);
$array1 = ['a' => 'b', 'c' => 'd', 'e' => 'f'];
$collection1 = Collection::init($array1)->slice(['c', 'a', 'g']);
if ($collection->toArray() != [3, 4] || $collection1->toArray() != ['c' => 'd', 'a' => 'b'])
    echo 'Collection::slice() failed.', PHP_EOL;
?>
--EXPECT--
