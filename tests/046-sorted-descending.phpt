--TEST--
Test Collection::sorted() and Collection::sortedDescending().
--FILE--
<?php
$array = [];
for ($i = 0; $i < 100; ++$i)
    $array[] = random_int(-200, 200);

$collection = Collection::init($array);
$array1 = $array;
sort($array1, SORT_NUMERIC);
if ($array1 != $collection->sorted()->toArray())
    echo 'Collection::sort() failed.', PHP_EOL;

$collection = Collection::init($array);
$array1 = $array;
rsort($array1, SORT_NUMERIC);
if ($array1 != $collection->sortedDescending()->toArray())
    echo 'Collection::sortDescending() failed.', PHP_EOL;
?>
--EXPECT--
