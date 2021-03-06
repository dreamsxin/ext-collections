--TEST--
Test Collection::groupBy() and Collection::groupByTo().
--FILE--
<?php
$collection = Collection::init([7, 2, 9, 4, 1, 0]);
$collection1 = $collection->groupBy(function ($value, $key) {
    if ($key % 2) {
        return 'odd_idx';
    }
    return 'even_idx';
});
$array = [
    'odd_idx' => [2, 4, 0],
    'even_idx' => [7, 9, 1]
];
if ($collection1->toArray() != $array) {
    echo 'Collection::groupByTo() failed.', PHP_EOL;
}

$collection = Collection::init([
    'a' => ['def', 12, 'ghi'],
    'b' => [71, 'jkl'],
    'c' => ['mno', 15]
]);
$array = [
    'integer' => [
        'a' => 'foo'
    ]
];
$collection1 = Collection::init($array);
$collection2 = $collection->groupByTo($collection1, function ($value) {
    if (is_int($value[1])) {
        return new Pair('integer', $value[0].strval($value[1]));
    }
    return 'string';
});
$array = [
    'integer' => [
        'a' => 'foo',
        'c' => 'mno15',
    ],
    'string' => [
        'b' => [71, 'jkl']
    ]
];
if ($collection2->toArray() != $array) {
    echo 'Collection::groupBy() failed.', PHP_EOL;
}
?>
--EXPECT--
