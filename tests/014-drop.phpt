--TEST--
Test Collection::drop() and Collection::dropLast().
--FILE--
<?php
$collection = Collection::init(['ab' => 'c', 'd', 'e' => 'f', 12, 34]);
$collection1 = $collection->drop(2);
$collection2 = $collection->dropLast(3);
if ($collection1->toArray() != ['e' => 'f', 1 => 12, 2 => 34])
    echo 'Collection::drop() failed.', PHP_EOL;
if ($collection2->toArray() != ['ab' => 'c', 'd'])
    echo 'Collection::dropLast() failed.', PHP_EOL;
?>
--EXPECT--
