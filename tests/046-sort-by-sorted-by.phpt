--TEST--
Test Collection::sortBy(), Collection::sortByDescending(), Collection::sortedBy(), Collection::sortedByDescending().
--FILE--
<?php
$array = [];
for ($i = 0; $i < 4; ++$i)
    $array[random_bytes(2)] = [random_int(10, 99)];

$sort_by = function ($value) {
    return strval($value[0]);
};
$sort_with = function ($v1, $v2) {
    return $v1[0] - $v2[0];
};
$sort_with_descending = function ($v1, $v2) {
    return $v2[0] - $v1[0];
};

$collection = Collection::init($array);
$collection->sortBy($sort_by);
$array1 = $array;
usort($array1, $sort_with);
if ($array1 != $collection->toArray())
    echo 'Collection::sortBy() failed.', PHP_EOL;

$collection = Collection::init($array);
$collection->sortByDescending($sort_by);
$array1 = $array;
usort($array1, $sort_with_descending);
if ($array1 != $collection->toArray())
    echo 'Collection::sortByDescending() failed.', PHP_EOL;

$collection = Collection::init($array);
$array1 = $array;
usort($array1, $sort_with);
if ($array1 != $collection->sortedBy($sort_by)->toArray())
    echo 'Collection::sortedBy() failed.', PHP_EOL;

$collection = Collection::init($array);
$array1 = $array;
usort($array1, $sort_with_descending);
if ($array1 != $collection->sortedByDescending($sort_by)->toArray())
    echo 'Collection::sortedByDescending() failed.', PHP_EOL;
?>
--EXPECT--
