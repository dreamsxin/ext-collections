--TEST--
Test Collection::average().
--FILE--
<?php
$array = [2.632, 7.871, 9.094, 5.457, 3, 9, 6];
$average = Collection::init($array)->average();
if ($average != array_sum($array) / count($array)) {
    echo 'Collection::average() failed.', PHP_EOL;
}
?>
--EXPECT--
