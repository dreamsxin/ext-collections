--TEST--
Test Collection::filter() and Collection::filterNot().
--FILE--
<?php
$array = [1, 3, 4, 5, 8, 10, 13];
$pred_is_odd = function ($value) {
    return $value % 2;
};
$pred_is_even = function ($value) {
    return $value % 2 == 0;
};
$collection = Collection::init($array)->filter($pred_is_odd);
$collection1 = Collection::init($array)->filterNot($pred_is_odd);
if ($collection->toArray() != array_filter($array, $pred_is_odd))
    echo 'Collection::filter() failed.', PHP_EOL;
if ($collection1->toArray() != array_filter($array, $pred_is_even))
    echo 'Collection::filter() failed.', PHP_EOL;
?>
--EXPECT--
