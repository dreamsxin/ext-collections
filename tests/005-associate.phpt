--TEST--
Test Collection::associate().
--FILE--
<?php
$array = ['foo' => 'bar', 34 => 5];
$collection = Collection::init($array);
$collection1 = $collection->associate(function ($value, $key) {
    return new Pair($value, $key);
});
if ($collection1->toArray() != array_flip($array))
    echo 'Collection::all() failed.', PHP_EOL;
?>
--EXPECT--
