#!/usr/bin/env php
<?php

echo "Status: 200 OK\n";
echo "Content-Type: text/html; charset=utf-8\n";
echo "\n";

$random = rand(1, 100);
?>
<html>
<head><title>PHP Random Number</title></head>
<body>
<h1>Random Number Generator</h1>
<p>Your lucky number is: <strong><?php echo $random; ?></strong></p>
</body>
</html>
