#!/usr/bin/env perl

use strict;
use warnings;
use CGI;

my $q = CGI->new;

# --- CGI headers ---
print "Status: 200 OK\n";
print "Content-Type: text/html; charset=utf-8\n";
print "\n";

my $name = $q->param('name') || 'Guest';

print <<"HTML";
<html>
<head><title>Perl Greeting CGI</title></head>
<body>
<h1>Greetings!</h1>
<p>Hello, <strong>$name</strong>!</p>

<form method="get">
    Enter your name: <input type="text" name="name">
    <input type="submit" value="Say Hello">
</form>
</body>
</html>
HTML
