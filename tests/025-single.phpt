--TEST--
Test Collection::single().
--FILE--
<?php
$array = ['a', 'b', 'cd', 'b', 'd', 'e'];
$collection = Collection::init($array);
$value = $collection->single(function ($value) {
    return $value == 'b';
});
$value1 = $collection->single(function ($value) {
    return strlen($value) == 2;
});
$value2 = $collection->single(function ($value, $key) {
    return $key > 5;
});
if (isset($value) || $value1 != 'cd' || isset($value2))
    echo 'Collection::single() failed.', PHP_EOL;
?>
--EXPECT--
