--TEST--
Test Collection::map() and Collection::mapTo().
--FILE--
<?php
$array = ['foo' => 'bar', 'bar' => 'baz'];
$collection = Collection::init($array)->map(function ($value, $key) {
    return $key.$value;
});
if ($collection->toArray() != ['foobar', 'barbaz'])
    echo 'Collection::map() failed.', PHP_EOL;
$collection1 = Collection::init(['dummy']);
$collection2 = Collection::init($array)->mapTo($collection1, function ($value, $key) {
    return $value.$key;
});
if ($collection1->toArray() != $collection2->toArray() || $collection1->toArray() != ['dummy', 'barfoo','bazbar'])
    echo 'Collection::mapTo() failed.', PHP_EOL;
?>
--EXPECT--
