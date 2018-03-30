--TEST--
Test Collection::count() and countable interface.
--FILE--
<?php
$array = [1, 2, 3, 4, 5, 6];
$collection = Collection::init($array);
if ($collection->count() != count($array))
    echo 'Collection::count() failed.', PHP_EOL;
if (count($collection) != count($array))
    echo 'Test for countable interface failed.', PHP_EOL;
?>
--EXPECT--
