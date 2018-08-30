--TEST--
Test accessing elements of Collection as object properties.
--FILE--
<?php
$collection = Collection::init([
    'a' => 'b',
    'c' => 'd',
    'e' => 0,
    'f' => null
]);
if (!property_exists($collection, 'e') || !property_exists($collection, 'f') ||
    !isset($collection->e) || isset($collection->f) ||
    !empty($collection->e) || !empty($collection->f)
) {
    echo 'Test for handlers.has_property failed.', PHP_EOL;
}
if ($collection->a != 'b') {
    echo 'Test for handlers.read_property failed.', PHP_EOL;
}
$collection->c = 'g';
if ($collection->c != 'g') {
    echo 'Test for handlers.write_property failed.', PHP_EOL;
}
unset($collection->e);
if (isset($collection->e)) {
    echo 'Test for handlers.unset_property failed.', PHP_EOL;
}
?>
--EXPECT--
