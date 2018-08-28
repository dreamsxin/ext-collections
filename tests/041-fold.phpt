--TEST--
Test Collection::fold() and Collection::foldRight().
--FILE--
<?php
$collection = Collection::init(['a', 'b', 'c', 'd', 'e']);
$reduce_cb = function ($acc, $value, $key) {
    return $acc.$value.strval($key);
};
if ($collection->fold('foo', $reduce_cb) != 'fooa0b1c2d3e4') {
    echo 'Collection::fold() failed.', PHP_EOL;
}
if ($collection->foldRight('bar', $reduce_cb) != 'bare4d3c2b1a0') {
    echo 'Collection::foldRight() failed.', PHP_EOL;
}
?>
--EXPECT--
