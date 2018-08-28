--TEST--
Test Collection::all() and Collection::any().
--FILE--
<?php
$collection = Collection::init(['abc' => 12, 'de' => 5, 'f' => 100]);
$result = $collection->all(function ($value) {
    return $value < 100;
});
$result1 = $collection->all(function ($value, $key) {
    return strlen($key) < 4;
});
if ($result || !$result1) {
    echo 'Collection::all() failed.', PHP_EOL;
}
$result = $collection->any(function ($value) {
    return $value % 3 == 0;
});
$result1 = $collection->any(function ($value, $key) {
    return strpos($key, 'g') !== false;
});
if (!$result || $result1) {
    echo 'Collection::any() failed.', PHP_EOL;
}
$result = $collection->none(function ($value) {
    return $value < 0;
});
$result1 = $collection->none(function ($value, $key) {
    return ctype_alnum($value . $key);
});
if (!$result || $result1) {
    echo 'Collection::none() failed.', PHP_EOL;
}
?>
--EXPECT--
