--TEST--
Test Collection::forEach() and Collection::onEach().
--FILE--
<?php
$array = ['a', 'b', 'c', 'd', 'e'];
$result3 = $result2 = $result1 = $result = '';
foreach ($array as $value) {
    $result .= $value;
}

$collection = Collection::init($array);
foreach ($collection as $value) {
    $result1 .= $value;
}
if ($result1 != $result) {
    echo 'Traversing Collection with foreach keyword failed.', PHP_EOL;
}

$collection->forEach(function ($value) use (&$result2) {
    $result2 .= $value;
});
if ($result2 != $result) {
    echo 'Collection::forEach() failed.', PHP_EOL;
}

$collection1 = $collection->onEach(function ($value) use (&$result3) {
    $result3 .= $value;
});
if ($result3 != $result || $collection1->toArray() != $collection->toArray()) {
    echo 'Collection::onEach() failed.', PHP_EOL;
}
?>
--EXPECT--
