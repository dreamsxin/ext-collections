--TEST--
Test Collection::min() and Collection::max().
--FILE--
<?php
$array = [3.42, 7.15, 0.0, -4.2, 1.64];
$collection = Collection::init($array);
if ($collection->max() != max($array) || Collection::init()->max() != null)
    echo 'Collection::max() failed.', PHP_EOL;
if ($collection->min() != min($array) || Collection::init()->min() != null)
    echo 'Collection::min() failed.', PHP_EOL;
?>
--EXPECT--
