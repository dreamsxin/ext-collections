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
$result1 = $collection->all(function ($value, $key) {
    return strlen($key) < 4;
});
$result2 = $collection->any(function ($value) {
    return $value % 3 == 0;
});
$result3 = $collection->any(function ($value, $key) {
    return strpos($key, 'g') !== false;
});
if ($result || !$result1 || !$result2 || $result3)
    echo 'Collection::all() failed.', PHP_EOL;
?>
--EXPECT--
