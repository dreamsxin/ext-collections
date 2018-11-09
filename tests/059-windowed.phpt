--TEST--
Test Collection::windowed().
--FILE--
<?php
$array = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'];
$collection = Collection::init($array);

$array1 = [
    ['a', 'b', 'c'],
    ['c', 'd', 'e'],
    ['e', 'f', 'g']
];
if ($collection->windowed(3, 2, false)->toArray() != $array1) {
    echo 'Collection::windowed() failed.', PHP_EOL;
}

$array1 = [
    ['a', 'b', 'c', 'd', 4],
    ['d', 'e', 'f', 'g', 4],
    ['g', 'h', 2]
];
$transform = function ($snapshot) {
    $snapshot[] = count($snapshot);
    return $snapshot;
};
if ($collection->windowed(4, 3, true, $transform)->toArray() != $array1) {
    echo 'Collection::windowed() failed.', PHP_EOL;
}
?>
--EXPECT--
