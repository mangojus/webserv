#!/usr/bin/env python3

import os

print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
# print("Connection: close")
print()

print("<html>")
print("<head><title>CGI Headers Debug</title></head>")
print("<body>")
print("<h1>CGI Request Debug</h1>")

print("<h2>Environment Variables</h2>")
print("<table border='1'>")

for key in sorted(os.environ.keys()):
    value = os.environ[key]
    print(f"<tr><td><b>{key}</b></td><td>{value}</td></tr>")

print("</table>")

print("</body>")
print("</html>")
