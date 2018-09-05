--TEST--
Test Collection::isEmpty().
--FILE--
<?php
if (!Collection::init()->isEmpty()) {
    echo 'Collection::isEmpty() failed.', PHP_EOL;
}
if (Collection::init(['foo', 'bar'])->isEmpty()) {
    echo 'Collection::isEmpty() failed.', PHP_EOL;
}
?>
--EXPECT--
