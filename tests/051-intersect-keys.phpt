--TEST--
Test Collection::intersect() and Collection::intersectKeys().
--FILE--
<?php
$array = [
    'a' => 'b',
    'c' => 'd',
    'e' => 'f'
];
$array1 = [
    'a' => 'c',
    'b' => strval(12),
    'c' => 'd',
    'g' => 'h'
];
$collection = Collection::init($array);
$intersected = array_intersect_assoc($array, $array1);
if ($collection->intersect($array1)->toArray() != $intersected) {
    echo 'Collection::intersect() failed.', PHP_EOL;
}
$collection1 = Collection::init($array1);
$intersected = array_intersect_key($array, $array1);
if ($collection->intersectKeys($collection1)->toArray() != $intersected) {
    echo 'Collection::intersectKeys() failed.', PHP_EOL;
}
?>
--EXPECT--
