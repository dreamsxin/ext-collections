--TEST--
Check whether the extension is loaded.
--FILE--
<?php
if (!extension_loaded('collections')) {
    echo "Extension not loaded.", PHP_EOL;
}
?>
--EXPECT--
