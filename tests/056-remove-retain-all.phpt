--TEST--
Test Collection::removeAll() and Collection::retainAll().
--FILE--
<?php
$array = [];
for ($i = 0; $i < 50; ++$i) {
    $array[] = mt_rand(1, 50);
}
$array1 = [];
for ($i = 0; $i < 50; ++$i) {
    $array1[] = mt_rand(1, 50);
}
$collection = Collection::init($array);

$collection1 = Collection::init($array1);
$intersected_with_duplicate = array_values(array_intersect($array1, $array));
$collection1->retainAll($collection);
if ($collection1->toArray() != $intersected_with_duplicate) {
    echo 'Collection::retainAll() failed.', PHP_EOL;
}

$collection1 = Collection::init($array1);
$subtracted_with_duplicate = array_values(array_diff($array1, $array));
$collection1->removeAll($collection);
if ($collection1->toArray() != $subtracted_with_duplicate) {
    echo 'Collection::removeAll() failed.', PHP_EOL;
}
?>
--EXPECT--
