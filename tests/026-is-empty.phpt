--TEST--
Test Collection::isEmpty() and Collection::isNotEmpty().
--FILE--
<?php
if (!Collection::init()->isEmpty())
    echo 'Collection::isEmpty() failed.', PHP_EOL;
if (!Collection::init(['foo', 'bar'])->isNotEmpty())
    echo 'Collection::isNotEmpty() failed.', PHP_EOL;
?>
--EXPECT--
