--TEST--
Test Collection::zipWithNext().
--FILE--
<?php
$array = ['foo'];
if (Collection::init($array)->zipWithNext()->toArray() != []) {
    echo 'Collection::zipWithNext() failed.', PHP_EOL;
}

$array = ['a', 'b', 'c', 'd'];
$collection = Collection::init($array);
$array1 = [
    new Pair('a', 'b'),
    new Pair('b', 'c'),
    new Pair('c', 'd')
];
if ($collection->zipWithNext()->toArray() != $array1) {
    echo 'Collection::zipWithNext() failed.', PHP_EOL;
}

$transform = function ($v1, $v2) {
    return $v1.$v2;
};
$array1 = ['ab', 'bc', 'cd'];
if ($collection->zipWithNext($transform)->toArray() != $array1) {
    echo 'Collection::zipWithNext() failed.', PHP_EOL;
}
?>
--EXPECT--
