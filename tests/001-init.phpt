--TEST--
Test Collection::init().
--FILE--
<?php
$array = ['a' => 'b'];
$collection = Collection::init($array);
$collection2 = Collection::init($collection);
if ($array != $collection2->toArray())
    echo 'Collection::init() failed.', PHP_EOL;
?>
--EXPECT--
