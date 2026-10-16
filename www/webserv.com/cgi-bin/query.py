#!/usr/bin/env python3

import os
import urllib.parse

print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
# print("Connection: close")
print()


query = os.environ.get("QUERY_STRING", "")
params = urllib.parse.parse_qs(query)

print("<html><body>")
print("<h1>Query Parameters</h1>")

for key, value in params.items():
    print(f"<p>{key} = {value}</p>")

print("</body></html>")
