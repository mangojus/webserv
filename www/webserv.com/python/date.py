#!/usr/bin/env python3

import datetime
from time import sleep

print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
print("Connection: close")
print("")

now = datetime.datetime.now()
# sleep(10)  # Simulate a long-running process
print(f"""<html>
<head><title>Python Date CGI</title></head>
<body>
<h1>Current Date & Time</h1>
<p>The server says: <strong>{now.strftime('%Y-%m-%d %H:%M:%S')}</strong></p>
</body>
</html>""")
