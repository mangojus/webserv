#!/usr/bin/env perl

use CGI;
my $q = CGI->new;

print $q->header('text/html');

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
