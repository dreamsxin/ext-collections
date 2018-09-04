--TEST--
Test Collection::removeWhile().
--FILE--
<?php
$array = ['a' => 4, 'b' => 1, 'c' => 9, 'd' => -2, 'e' => 0];
$pred_is_odd = function ($value) {
    return $value % 2;
};
$collection = Collection::init($array);
$collection->removeWhile($pred_is_odd);
if ($collection->toArray() != ['a' => 4, 'd' => -2, 'e' => 0]) {
    echo 'Collection::removeWhile() failed.', PHP_EOL;
}
?>
--EXPECT--
