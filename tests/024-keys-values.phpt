--TEST--
Test Collection::keys() and Collection::values().
--FILE--
<?php
$array = ['a' => 'b', 'c' => ['d'], 10 => 'f'];
$collection = Collection::init($array);
if ($collection->keys()->toArray() != array_keys($array))
    echo 'Collection::keys() failed.', PHP_EOL;
if ($collection->values()->toArray() != array_values($array))
    echo 'Collection::values() failed.', PHP_EOL;
?>
--EXPECT--
