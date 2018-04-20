--TEST--
Test Collection::toCollection().
--FILE--
<?php
$collection = Collection::init(['a', 'b']);
Collection::init(['foo', 'bar'])->toCollection($collection);
if ($collection->toArray() != ['a', 'b', 'foo', 'bar'])
    echo 'Collection::toCollection() failed.', PHP_EOL;
?>
--EXPECT--
