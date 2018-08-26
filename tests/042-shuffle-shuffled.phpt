--TEST--
Test Collection::shuffle() and Collection::shuffled().
--FILE--
<?php
$array = ['a' => 1, 'b' => 2, 'c' => 3, 'd' => 4, 'e' => 5];

for ($i = 0; ; ++$i) {
    $collection = Collection::init($array);
    $collection->shuffle();
    if ($collection->toArray() != array_values($array))
        break;
    if ($i > 10) {
        echo 'Collection::shuffle() failed.', PHP_EOL;
        exit;
    }
}
if (array_sum($array) != array_sum($collection->toArray()))
    echo 'Collection::shuffle() failed.', PHP_EOL;

for ($i = 0; ; ++$i) {
    $shuffled = Collection::init($array)->shuffled();
    if ($shuffled->toArray() != array_values($array))
        break;
    if ($i > 10) {
        echo 'Collection::shuffled() failed.', PHP_EOL;
        exit;
    }
}
if (array_sum($array) != array_sum($shuffled->toArray()))
    echo 'Collection::shuffled() failed.', PHP_EOL;
?>
--EXPECT--
