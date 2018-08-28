--TEST--
Test Collection::flatMap() and Collection::flatMapTo().
--FILE--
<?php
$array = [['a' => ['b', 'c'], 'd' => ['e', 'f']], ['a' => ['g', 'h'], 'd' => ['i', 'j']]];
$collection = Collection::init($array)->flatMap(function ($value) {
    return $value['a'];
});
if ($collection->toArray() != ['b', 'c', 'g', 'h']) {
    echo 'Collection::flatMap() failed.', PHP_EOL;
}
$collection1 = Collection::init(['k', 'l']);
$collection2 = Collection::init($array)->flatMapTo($collection1, function ($value) {
    return Collection::init($value['d']);
});
if ($collection2->toArray() != ['k', 'l', 'e', 'f', 'i', 'j']) {
    echo 'Collection::flatMap() failed.', PHP_EOL;
}
?>
--EXPECT--
