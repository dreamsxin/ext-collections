--TEST--
Test Collection::takeWhile() and Collection::takeLastWhile().
--FILE--
<?php
$array = ['a', 'b', 'C', 'D', 'e'];
$collection = Collection::init($array);
$collection1 = $collection->takeWhile(function ($value) {
    return ord($value) > ord('Z');
});
if ($collection1->toArray() != ['a', 'b'])
    echo 'Collection::takeWhile() failed.', PHP_EOL;
$collection2 = $collection->takeLastWhile(function ($value) {
    return $value != 'b';
});
if ($collection2->toArray() != ['C', 'D', 'e'])
    echo 'Collection::takeLastWhile() failed.', PHP_EOL;
?>
--EXPECT--
