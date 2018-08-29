--TEST--
Test Collection::distinctBy().
--FILE--
<?php
$array = [];
for ($i = 0; $i < 100; ++$i) {
    $array[] = random_bytes(random_int(1, 10));
}
$get_len = function ($value) {
    return strlen($value);
};
$distinct = Collection::init($array)->distinctBy($get_len)->toArray();
$array1 = [];
$distinct_idx = array_keys(array_unique(array_map($get_len, $array)));
foreach ($distinct_idx as $idx) {
    $array1[] = $array[$idx];
}
if ($distinct != $array1) {
    echo 'Collection::distinct() failed.', PHP_EOL;
}
?>
--EXPECT--
