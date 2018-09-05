--TEST--
Test Collection::minus().
--FILE--
<?php
$array = [
    'a' => 'b',
    'c' => 'd',
    'e' => 'f'
];
$array1 = [
    'a' => 'g',
    'c' => 'd',
    'h' => 'f'
];
$collection = Collection::init($array);
$collection1 = Collection::init($array1);
$subtraction = array_diff_assoc($array, $array1);
if ($collection->minus($collection1)->toArray() != $subtraction) {
    echo 'Collection::minus() failed.', PHP_EOL;
}
?>
--EXPECT--
