--TEST--
Test Collection::removeAll() and Collection::retainAll().
--FILE--
<?php
$array = ['a' => 4, 'b' => 1, 'c' => 9, 'd' => -2, 'e' => 0];
$pred_is_odd = function ($value) {
    return $value % 2;
};
$collection = Collection::init($array);
$collection->removeAll($pred_is_odd);
if ($collection->toArray() != ['a' => 4, 'd' => -2, 'e' => 0])
    echo 'Collection::removeAll() failed.', PHP_EOL;
$collection->removeAll();
if ($collection->toArray() != [])
    echo 'Collection::removeAll() failed.', PHP_EOL;
$collection = Collection::init($array);
$collection->retainAll($pred_is_odd);
if ($collection->toArray() != ['b' => 1, 'c' => 9])
    echo 'Collection::retainAll() failed.', PHP_EOL;
?>
--EXPECT--
