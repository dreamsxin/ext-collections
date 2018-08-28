--TEST--
Test Pair.
--FILE--
<?php
$pair = new Pair('foo', 'bar');
if ($pair->first != 'foo' || $pair->second != 'bar') {
    echo 'Test for Pair failed.', PHP_EOL;
}
?>
--EXPECT--
