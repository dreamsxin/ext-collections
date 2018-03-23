--TEST--
Test Collection::addAll().
--FILE--
<?php
$array = [3, 7, 5, 6, 1, 2];
$collection = Collection::init([6, 1, 2]);
$collection2 = Collection::init([3, 7, 5]);
$collection2->addAll($collection);
if ($array != $collection2->toArray())
    echo 'Collection::addAll() failed.', PHP_EOL;
?>
--EXPECT--
