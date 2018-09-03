--TEST--
Test Collection::containsAll(), Collection::containsAllKeys(), Collection::containsAllValues().
--FILE--
<?php
$collection = Collection::init([
    'a' => 'b',
    'c' => 'd',
    'e' => 'f'
]);
$collection1 = Collection::init([
    'a' => 'j',
    'c' => 'f',
    'e' => 'b'
]);
$collection2 = Collection::init([
    'a' => 'b',
    'e' => 'f'
]);
if (!$collection->containsAll($collection2) ||
    $collection2->containsAll($collection) ||
    $collection->containsAll($collection1)
) {
    echo 'Collection::containsAll() failed.', PHP_EOL;
}
if (!$collection1->containsAllKeys($collection2) ||
    !$collection->containsAllKeys($collection1) ||
    $collection2->containsAllKeys($collection1)
) {
    echo 'Collection::containsAllKeys() failed.', PHP_EOL;
}
if (!$collection->containsAllValues($collection2) ||
    !$collection1->containsAllValues($collection2) ||
    $collection->containsAllValues($collection1)
) {
    echo 'Collection::containsAllValues() failed.', PHP_EOL;
}
?>
--EXPECT--
