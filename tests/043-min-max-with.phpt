--TEST--
Test Collection::minWith() and Collection::maxWith().
--FILE--
<?php
$array = [
    'a' => ['b' => 2, 'c' => 6],
    'd' => ['b' => 3, 'c' => 1],
    'e' => ['b' => 5, 'c' => 4]
];
$collection = Collection::init($array);
$by_b = function ($p1, $p2) {
    return $p1->second['b'] - $p2->second['b'];
};
$by_c = function ($p1, $p2) {
    return strval($p1->second['c'] - $p2->second['c']);
};
if ($collection->minWith($by_b) != $array['a'] || $collection->minWith($by_c) != $array['d'])
    echo 'Collection::minWith() failed.', PHP_EOL;
if ($collection->maxWith($by_b) != $array['e'] || $collection->maxWith($by_c) != $array['a'])
    echo 'Collection::maxWith() failed.', PHP_EOL;
?>
--EXPECT--
