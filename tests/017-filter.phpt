--TEST--
Test Collection::filter(), Collection::filterNot(), Collection::filterTo(), Collection::filterNotTo().
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
    echo 'Collection::filterNot() failed.', PHP_EOL;
$dest = Collection::init($array);
$collection2 = Collection::init($array)->filterTo($dest, $pred_is_odd);
if ($collection2->toArray() != array_merge($array, $collection->toArray()))
    echo 'Collection::filterTo() failed.', PHP_EOL;
$collection3 = Collection::init($array)->filterNotTo($dest, $pred_is_odd);
if ($collection3->toArray() != array_merge($array, $collection->toArray(), $collection1->toArray()))
    echo 'Collection::filterNotTo() failed.', PHP_EOL;
?>
--EXPECT--
