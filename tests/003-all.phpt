--TEST--
Test Collection::all().
--FILE--
<?php
$collection = Collection::init([
    "abc" => 12,
    "de" => 5,
    "f" => 100
]);
$result = $collection->all(function ($value) {
    return $value < 100;
});
$result2 = $collection->all(function ($value, $key) {
    return strlen($key) < 4;
});
if ($result || !$result2)
    echo 'Collection::all() failed.', PHP_EOL;
?>
--EXPECT--
