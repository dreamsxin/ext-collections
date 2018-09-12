--TEST--
Test Collection::binarySearch().
--FILE--
<?php
$array = [];
$size = random_int(20, 30);
for ($i = 0; $i < 50; ++$i) {
    $array[] = random_int(100, 200);
}
$array = array_unique($array);
sort($array);
$idx = random_int(0, count($array) - 1);
$which = $array[$idx];
$from = random_int(0, $idx);
$to = random_int($idx, count($array));
$collection = Collection::init($array);
if ($collection->binarySearch($which, 0, $from, $to) != $idx) {
    echo 'Collection::binarySearch() failed.', PHP_EOL;
}
$array = array_map(function ($value) {
    return [$value];
}, $array);
$selector = function ($value) {
    return $value[0];
};
if ($collection->binarySearchBy($which, $selector, 0, $from, $to) != $idx) {
    echo 'Collection::binarySearchBy() failed.', PHP_EOL;
}
?>
--EXPECT--
