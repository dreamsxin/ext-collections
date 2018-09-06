--TEST--
Test Collection::sum() and Collection::sumBy().
--FILE--
<?php
$array = [];
for ($i = 0; $i < 50; ++$i) {
    $array[] = random_int(1, 50);
}
$collection = Collection::init($array);
$sum = array_sum($array);
if ($collection->sum() != $sum) {
    echo 'Collection::sum() failed.', PHP_EOL;
}
$array = array_map(function ($value) {
    return [$value, floatval($value / random_int(3, 7))];
}, $array);
$collection = Collection::init($array);
$sum = array_sum(array_column($array, 1));
$sum_by = function ($value) {
    return $value[1];
};
if ($collection->sumBy($sum_by) != $sum) {
    echo 'Collection::sumBy() failed.', PHP_EOL;
}
?>
--EXPECT--
