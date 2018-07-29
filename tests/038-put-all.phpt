--TEST--
Test Collection::putAll().
--FILE--
<?php
$array = ['a' => 'foo', 'b' => 'bar'];
$array1 = ['a' => 'b', 'c' => 'd', 'e' => 'f'];
$collection = Collection::init($array);
$collection->putAll($array1);
if ($collection->toArray() != array_merge($array, $array1))
    echo 'Collection::putAll() failed.', PHP_EOL;
?>
--EXPECT--
