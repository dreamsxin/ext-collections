--TEST--
Test Collection::drop() and Collection::dropLast().
--FILE--
<?php
$collection = Collection::init(['a', 'b', 'c', 'd', 'e']);
$collection1 = $collection->drop(3);
if (array_values($collection1->toArray()) != ['d', 'e']) {
    echo 'Collection::drop() failed.', PHP_EOL;
}
$collection1 = $collection->dropLast(2);
if ($collection1->toArray() != ['a', 'b', 'c']) {
    echo 'Collection::dropLast() failed.', PHP_EOL;
}
?>
--EXPECT--
