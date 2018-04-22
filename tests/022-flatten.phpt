--TEST--
Test Collection::flatten().
--FILE--
<?php
$array = [['a', 'foo' => 'b'], ['c', 'd', ['e']], 'bar' => 'f'];
$collection = Collection::init($array)->flatten();
if ($collection->toArray() != ['a', 'foo' => 'b', 'c', 'd', ['e'], 'bar' => 'f'])
    echo 'Collection::flatten() failed.', PHP_EOL;
?>
--EXPECT--
