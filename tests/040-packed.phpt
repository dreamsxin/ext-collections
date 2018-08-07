--TEST--
Test Collection::packed().
--FILE--
<?php
$case = Collection::init(['a', 'b', 'c'])->packed();
$case1 = Collection::init(['foo' => 'bar', 'baz'])->packed();
$case2 = Collection::init()->packed();
if (!$case || $case1 || $case2)
    echo 'Collection::packed() failed.', PHP_EOL;
?>
--EXPECT--
