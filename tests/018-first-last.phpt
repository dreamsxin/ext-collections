--TEST--
Test Collection::first() and Collection::last().
--FILE--
<?php
$array = [-5, -2, 1, 3, 5, 8, 9];
$collection = Collection::init($array);
$collection1 = Collection::init();
$value = $collection->first(function ($value) {
    return $value > 0;
});
if ($value != 1 || $collection->first() != -5 || $collection1->first() != null) {
    echo 'Collection::first() failed.', PHP_EOL;
}
$value1 = $collection->last(function ($value) {
    return $value % 2 == 0;
});
if ($value1 != 8 || $collection->last() != 9 || $collection1->last() != null) {
    echo 'Collection::last() failed.', PHP_EOL;    
}
?>
--EXPECT--
