#!/usr/bin/env php
<?php

echo "Status: 200 OK\n";
echo "Content-Type: text/html; charset=utf-8\n";
echo "\n";

echo "<html><body>";
echo "<h1>Hello from PHP CGI</h1>";
echo "<p>Time: " . date("Y-m-d H:i:s") . "</p>";
echo "</body></html>";

?>
