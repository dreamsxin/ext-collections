--TEST--
Test Collection::containsKey() and Collection::containsValue().
--FILE--
<?php
$collection = Collection::init(['foo' => 'bar', 12 => 6, 'baz' => 27]);
if (!$collection->containsKey('foo') || $collection->containsKey('Baz')) {
    echo 'Collection::containsKey() failed.', PHP_EOL;
}
if (!$collection->containsValue(27) || $collection->containsValue(3)) {
    echo 'Collection::containsValue() failed.', PHP_EOL;
}
?>
--EXPECT--
