--TEST--
Test Collection::copyOf();
--FILE--
<?php
$array = ['a' => 2, 3 => 'd', 'e' => 'f'];
$collection = Collection::init($array);
$collection1 = $collection->copyOf(2);
if ($collection1->toArray() != array_slice($array, 0, 2, true)) {
    echo 'Collection::copyOf() failed.', PHP_EOL;
}
$collection1 = $collection->copyOf();
if ($collection1->toArray() != $array) {
    echo 'Collection::copyOf() failed.', PHP_EOL;
}
?>
--EXPECT--
