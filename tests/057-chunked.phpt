--TEST--
Test Collection::chunked().
--FILE--
<?php
$array = [];
$size = mt_rand(20, 30);
for ($i = 0; $i < 50; ++$i) {
    $array[] = mt_rand(100, 200);
}
$chunk_size = mt_rand(4, 9);
$chunked = array_chunk($array, $chunk_size, false);
$collection = Collection::init($array);
if ($collection->chunked($chunk_size)->toArray() != $chunked) {
    echo 'Collection::chunked() failed.', PHP_EOL;
}
$transform = function ($value) use ($chunk_size) {
    if (count($value) < $chunk_size) {
        return [];
    }
    return $value;
};
$chunked = array_map($transform, $chunked);
if ($collection->chunked($chunk_size, $transform)->toArray() != $chunked) {
    echo 'Collection::chunked() failed.', PHP_EOL;
}
?>
--EXPECT--
