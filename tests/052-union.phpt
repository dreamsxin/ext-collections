--TEST--
Test Collection::union().
--FILE--
<?php
$array = [];
for ($i = 0; $i < 50; ++$i) {
    $array[] = mt_rand(1, 50);
}
$array1 = [];
for ($i = 0; $i < 50; ++$i) {
    $array1[] = mt_rand(1, 50);
}
$union = array_values(array_unique(array_merge($array, $array1)));
$collection = Collection::init($array);
$collection1 = Collection::init($array1);
if ($collection->union($collection1)->toArray() != $union) {
    echo 'Collection::union() failed.', PHP_EOL;
}
?>
--EXPECT--
