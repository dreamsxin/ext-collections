--TEST--
Test Collection::minBy() and Collection::maxBy().
--FILE--
<?php
$array = [
    [1.2, 3.1, 1.5],
    [3.2, 7.6, 4.7],
    [1.5, 1.4, 6.3]
];
$collection = Collection::init($array);
$result = $collection->maxBy(function ($value) {
    return $value[0];
});
if ($result != $array[1])
    echo 'Collection::maxBy() failed.', PHP_EOL;
$result = $collection->minBy(function ($value) {
    return $value[2];
});
if ($result != $array[0])
    echo 'Collection::minBy() failed.', PHP_EOL;
?>
--EXPECT--
