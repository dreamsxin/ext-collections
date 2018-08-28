--TEST--
Test Collection::init().
--FILE--
<?php
$array = ['a' => 'b'];
$collection = Collection::init($array);
$collection1 = Collection::init($collection);
$collection2 = Collection::init();
if ($array != $collection1->toArray() || $collection2->toArray() != []) {
    echo 'Collection::init() failed.', PHP_EOL;
}
?>
--EXPECT--
