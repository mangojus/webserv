#!/usr/bin/env python3
import os
import sys

length = int(os.environ.get("CONTENT_LENGTH", "0"))
body = sys.stdin.read(length)

print("Content-Type: text/html")
print()
print("<h1>POST received</h1>")
print(body)