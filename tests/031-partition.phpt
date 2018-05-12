--TEST--
Test Collection::partition().
--FILE--
<?php
$array = ['abc', 'd', 'ef', 'ghij'];
$pair = Collection::init($array)->partition(function ($value) {
    return strlen($value) % 2;
});
if ($pair->first != ['abc', 'd'] || $pair->second != [2 => 'ef', 3 => 'ghij'])
    echo 'Collection::partition() failed.', PHP_EOL;
?>
--EXPECT--
