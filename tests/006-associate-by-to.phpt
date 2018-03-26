--TEST--
Test Collection::associateBy() and Collection::associateByTo().
--FILE--
<?php
$array = ['a' => 'b', 'cd' => 'd', 'fgh' => 'i'];
$collection = Collection::init($array)
    ->associateBy(function ($value, $key) {
        return strlen($key) - 1;
    });
if ($collection->toArray() != array_values($array))
    echo 'Collection::associateBy() failed.', PHP_EOL;
$array1 = ['foo' => 'bar'];
$collection1 = Collection::init($array1);
$array2 = $collection->associateByTo($collection1, function ($value, $key) {
    return $key;
})->toArray();
if ($array2 != $array1 + $collection->toArray() || $collection1->toArray() != $array2)
    echo 'Collection::associateTo() failed.', PHP_EOL;
?>
--EXPECT--
