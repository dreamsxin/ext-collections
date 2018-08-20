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
$array1 = [
    ['abc', 'ABD'],
    ['ACE', 'ABC'],
    ['acd', 'aba']
];
$collection1 = Collection::init($array1);
$result = $collection1->maxBy(function ($value) {
    return $value[0];
});
if ($result != $array1[2])
    echo 'Collection::maxBy() failed.', PHP_EOL;
$result = $collection1->minBy(function ($value) {
    return $value[1];
}, Collection::FOLD_CASE);
if ($result != $array1[2])
    echo 'Collection::minBy() failed.', PHP_EOL;
?>
--EXPECT--
