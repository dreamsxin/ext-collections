--TEST--
Test Collection::toPairs().
--FILE--
<?php
$pairs = Collection::init(['a' => 'b', 'c' => 'd'])->toPairs();
$test = '';
foreach ($pairs as $pair) {
    $test .= $pair->first . ',' . $pair->second . ';';
}
if ($test != 'a,b;c,d;') {
    echo 'Collection::toPairs() failed.', PHP_EOL;
}
?>
--EXPECT--
