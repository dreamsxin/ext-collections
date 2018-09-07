--TEST--
Test Collection::intersectValues() and Collection::subtract().
--FILE--
<?php
$array = [];
for ($i = 0; $i < 50; ++$i) {
    $array[] = random_int(1, 50);
}
$array1 = [];
for ($i = 0; $i < 50; ++$i) {
    $array1[] = random_int(1, 50);
}
$collection = Collection::init($array);
$collection1 = Collection::init($array1);

$intersected = array_values(array_unique(array_intersect($array, $array1)));
if ($collection->intersectValues($collection1)->toArray() != $intersected ||
    $collection->intersectValues([])->toArray() != [] ||
    Collection::init()->intersectValues($collection1)->toArray() != []
) {
    echo 'Collection::intersectValues() failed.', PHP_EOL;
}

$subtracted = array_values(array_unique(array_diff($array, $array1)));
if ($collection->subtract($collection1)->toArray() != $subtracted ||
    $collection->subtract([])->toArray() != $array ||
    $collection->subtract($collection)->toArray() != []
) {
    echo 'Collection::subtract() failed.', PHP_EOL;
}
?>
--EXPECT--
