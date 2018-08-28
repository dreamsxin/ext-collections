--TEST--
Test Collection::get() and Collection::set().
--FILE--
<?php
$array = ['a' => 'b', 'c' => 'd'];
$collection = Collection::init($array);
$collection->set('a', 'foo');
$collection->set('e', 'bar');
$array['a'] = 'foo';
$array['e'] = 'bar';
if ($collection->toArray() != $array) {
    echo 'Collection::set() failed.', PHP_EOL;
}
if ($collection->get('a') != $array['a'] ||
    $collection->get('f', function ($key) { return $key; }) != 'f'
) {
    echo 'Collection::get() failed.', PHP_EOL;
}
if (!$collection->remove('a') || !is_null($collection->get('a'))) {
    echo 'Collection::remove() failed.', PHP_EOL;
}
if ($collection->remove('c', 'e') || $collection->get('c') != 'd') {
    echo 'Collection::remove() failed.', PHP_EOL;
}
?>
--EXPECT--
