--TEST--
Test Collection::isPacked().
--FILE--
<?php
$case = Collection::init(['a', 'b', 'c'])->isPacked();
$case1 = Collection::init(['foo' => 'bar', 'baz'])->isPacked();
$case2 = Collection::init()->isPacked();
if (!$case || $case1 || $case2) {
    echo 'Collection::isPacked() failed.', PHP_EOL;
}
?>
--EXPECT--
