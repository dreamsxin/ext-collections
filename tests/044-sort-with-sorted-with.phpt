--TEST--
Test Collection::sortWith() and Collection::sortedWith().
--FILE--
<?php
$array = [
    'a' => ['b' => 2, 'c' => 6],
    'd' => ['b' => 3, 'c' => 1],
    'e' => ['b' => 5, 'c' => 4],
    'f' => ['b' => 1, 'c' => 3]
];
$collection = Collection::init($array);
$by_b = function ($p1, $p2) {
    return $p1->second['b'] - $p2->second['b'];
};
$usort_by_b = function ($v1, $v2) {
    return $v1['b'] - $v2['b'];
};
$sorted_by_b = $collection->sortedWith($by_b);
$array1 = $array;
usort($array1, $usort_by_b);
if ($sorted_by_b->toArray() != $array1) {
    echo 'Collection::sortedWith() failed.', PHP_EOL;
}
$by_c = function ($p1, $p2) {
    return $p1->second['c'] - $p2->second['c'];
};
$usort_by_c = function ($v1, $v2) {
    return $v1['c'] - $v2['c'];
};
$collection->sortWith($by_c);
$array2 = $array;
usort($array2, $usort_by_c);
if ($collection->toArray() != $array2) {
    echo 'Collection::sortedWith() failed.', PHP_EOL;
}
?>
--EXPECT--
