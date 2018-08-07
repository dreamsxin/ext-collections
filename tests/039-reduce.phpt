--TEST--
Test Collection::reduce() and Collection::reduceRight().
--FILE--
<?php
$collection = Collection::init(['a', 'b', 'c', 'd', 'e']);
$reduce_cb = function ($acc, $value, $key) {
    return $acc.$value.strval($key);
};
if ($collection->reduce($reduce_cb) != 'ab1c2d3e4')
    echo 'Collection::reduce() failed.', PHP_EOL;
if ($collection->reduceRight($reduce_cb) != 'ed3c2b1a0')
    echo 'Collection::reduceRight() failed.', PHP_EOL;
?>
--EXPECT--
