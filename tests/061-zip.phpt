--TEST--
Test Collection::zip().
--FILE--
<?php
$array = ['a', 'b', 'c'];
$array1 = ['d', 'e'];
$zipped = Collection::init($array)->zip($array1)->toArray();
if ($zipped != [new Pair('a', 'd'), new Pair('b', 'e')]) {
    echo 'Collection::zip() failed.', PHP_EOL;
}

$transform = function ($v1, $v2) {
    return $v1.$v2;
};
$zipped = Collection::init($array1)->zip($array, $transform)->toArray();
if ($zipped != ['da', 'eb']) {
    echo 'Collection::zip() failed.', PHP_EOL;
}
?>
--EXPECT--
