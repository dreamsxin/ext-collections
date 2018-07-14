--TEST--
Test Collection::indexOf(), Collection::lastIndexOf(), Collection::indexOfFirst(), Collection::indexOfLast().
--FILE--
<?php
$collection = Collection::init(['t', 'B', 13, 'f', 'G', 19, 'N', 'f']);
$first_f_at = $collection->indexOf('f');
if ($first_f_at != 3)
    echo 'Collection::indexOf() failed.', PHP_EOL;
$last_f_at = $collection->lastIndexOf('f');
if ($last_f_at != 7)
    echo 'Collection::lastIndexOf() failed.', PHP_EOL;
$first_numeric_at = $collection->indexOfFirst(function ($value) {
    return is_numeric($value);
});
if ($first_numeric_at != 2)
    echo 'Collection::indexOfFirst() failed.', PHP_EOL;
$last_upper_case_at = $collection->indexOfLast(function ($value) {
    return ctype_upper($value);
});
if ($last_upper_case_at != 6)
    echo 'Collection::indexOfLast() failed.', PHP_EOL;
?>
--EXPECT--
