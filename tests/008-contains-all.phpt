--TEST--
Test Collection::containsAll().
--FILE--
<?php
$array = [[], 1, 't'];
$array1 = [2, ['t'], 5];
$collection = Collection::init([1, 2, 't', [], 5]);
if (!$collection->containsAll($array) || $collection->containsAll($array1)) {
    echo 'Collection::containsAll() failed.', PHP_EOL;
}
?>
--EXPECT--
