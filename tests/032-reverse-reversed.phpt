--TEST--
Test Collection::reverse() and Collection::reversed().
--FILE--
<?php
$array = ['a', 'b', 'c', 'd', 'e'];
$collection = Collection::init($array);
$collection->reverse();
if ($collection->toArray() != array_reverse($array))
    echo 'Collection::reverse() failed.', PHP_EOL;
if ($collection->reversed()->toArray() != $array)
    echo 'Collection::reversed() failed.', PHP_EOL;
?>
--EXPECT--
