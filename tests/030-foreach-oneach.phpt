--TEST--
Test Collection::forEach() and Collection::onEach().
--FILE--
<?php
$array = ['a', 'b', 'c', 'd', 'e'];
$result2 = $result1 = $result = '';
foreach ($array as $value)
    $result .= $value;
$collection = Collection::init($array);
$collection->forEach(function ($value) use (&$result1) {
    $result1 .= $value;
});
$collection1 = $collection->onEach(function ($value) use (&$result2) {
    $result2 .= $value;
});
if ($result1 != $result)
    echo 'Collection::forEach() failed.', PHP_EOL;
if ($result2 != $result || $collection1->toArray() != $collection->toArray())
    echo 'Collection::onEach() failed.', PHP_EOL;
?>
--EXPECT--
