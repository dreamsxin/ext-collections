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
$array1 = ['p3.4', 'p3.32', 'p10.2'];
$collection1 = Collection::init($array1);
if ($collection1->max() != $array1[0] || $collection1->max(Collection::COMPARE_NATRUAL) != $array1[2])
    echo 'Collection::max() failed.', PHP_EOL;
if ($collection1->min() != $array1[2] || $collection1->min(Collection::COMPARE_NATRUAL) != $array1[0])
    echo 'Collection::min() failed.', PHP_EOL;
?>
--EXPECT--
