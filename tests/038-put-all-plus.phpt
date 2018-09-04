--TEST--
Test Collection::putAll().
--FILE--
<?php
$array = ['a' => 'foo', 'b' => 'bar'];
$array1 = ['a' => 'b', 'c' => 'd', 'e' => 'f'];
$collection = Collection::init($array);
$collection->putAll($array1);
$merged = array_merge($array, $array1);
if ($collection->toArray() != $merged) {
    echo 'Collection::putAll() failed.', PHP_EOL;
}
$array2 = ['e' => 'g'];
$collection1 = Collection::init($array2);
if ($collection->plus($collection1)->toArray() != array_merge($merged, $array2)) {
    echo 'Collection::putAll() failed.', PHP_EOL;
}
?>
--EXPECT--
