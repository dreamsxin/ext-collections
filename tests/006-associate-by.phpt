--TEST--
Test Collection::associateBy() and Collection::associateByTo().
--FILE--
<?php
$array = ['a' => 'b', 'cd' => 'd', 'fgh' => 'i'];
$collection = Collection::init($array)
    ->associateBy(function ($value, $key) {
        return strlen($key) - 1;
    });
if ($collection->toArray() != array_values($array)) {
    echo 'Collection::associateBy() failed.', PHP_EOL;
}
$array = ['foo' => 'bar'];
$collection = Collection::init($array);
$array1 = $collection->associateByTo($collection, function ($value, $key) {
    return $key;
})->toArray();
if ($array1 != $array + $collection->toArray() || $collection->toArray() != $array1) {
    echo 'Collection::associateTo() failed.', PHP_EOL;
}
?>
--EXPECT--
