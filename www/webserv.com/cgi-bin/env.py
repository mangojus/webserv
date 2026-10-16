#!/usr/bin/env python3

import os

print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
# print("Connection: close")
print()

print("<html><body>")
print("<h1>CGI Environment</h1>")

for key, value in os.environ.items():
    print(f"<p><b>{key}</b>: {value}</p>")

print("</body></html>")
