--TEST--
Test Collection::min() and Collection::max().
--FILE--
<?php
$array = [3.42, 7.15, 0.0, -4.2, 1.64];
$collection = Collection::init($array);
if ($collection->max() != max($array) || Collection::init()->max() != null) {
    echo 'Collection::max() failed.', PHP_EOL;
}
if ($collection->min() != min($array) || Collection::init()->min() != null) {
    echo 'Collection::min() failed.', PHP_EOL;
}

$array = ['p3.4', 'p3.32', 'p10.2'];
$collection = Collection::init($array);
if ($collection->max() != $array[0] || $collection->max(Collection::COMPARE_NATRUAL) != $array[2]) {
    echo 'Collection::max() failed.', PHP_EOL;
}
if ($collection->min() != $array[2] || $collection->min(Collection::COMPARE_NATRUAL) != $array[0]) {
    echo 'Collection::min() failed.', PHP_EOL;
}
?>
--EXPECT--
