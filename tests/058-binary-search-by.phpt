--TEST--
Test Collection::binarySearch() and Collection::binarySearchBy().
--FILE--
<?php
$array = [];
$size = mt_rand(20, 30);
for ($i = 0; $i < 50; ++$i) {
    $array[] = mt_rand(100, 200);
}
$array = array_unique($array);
sort($array);
$idx = mt_rand(0, count($array) - 1);
$which = $array[$idx];
$from = mt_rand(0, $idx);
$to = mt_rand($idx, count($array));
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
