--TEST--
Test Collection::count() and implementation of interface Countable.
--FILE--
<?php
$array = [1, 2, 3, 4, 5, 6];
$collection = Collection::init($array);
if ($collection->count() != count($array))
    echo 'Collection::count() failed.', PHP_EOL;
if (count($collection) != count($array))
    echo 'Test for handlers.count_elements failed.', PHP_EOL;
?>
--EXPECT--
