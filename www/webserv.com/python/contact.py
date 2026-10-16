#!/usr/bin/env python3

import os
import sys
from urllib.parse import parse_qs
import datetime

# Lire le body POST
content_length = int(os.environ.get("CONTENT_LENGTH", 0))
post_data = sys.stdin.read(content_length)

# Parser les données
data = parse_qs(post_data)

name = data.get("name", [""])[0]
email = data.get("email", [""])[0]
topic = data.get("topic", [""])[0]
message = data.get("message", [""])[0]

# Générer nom de fichier avec timestamp
now = datetime.datetime.now()
timestamp = now.strftime("%Y%m%d_%H%M%S")

directory = "www/webserv.com/data"
filename = f"{directory}/contact_{timestamp}.txt"

# Créer dossier si nécessaire
os.makedirs(directory, exist_ok=True)

# Écrire dans le fichier
with open(filename, "w") as f:
    f.write(f"Nom: {name}\n")
    f.write(f"Email: {email}\n")
    f.write(f"Sujet: {topic}\n")
    f.write(f"Message:\n{message}\n")

# Réponse CGI
print("Content-Type: text/html; charset=utf-8")
print()

print(f"""<html>
<head>
    <title>Confirmation</title>
</head>
<body>
    <h1>Message reçu ✅</h1>
    <p><strong>Nom :</strong> {name}</p>
    <p><strong>Email :</strong> {email}</p>
    <p><strong>Sujet :</strong> {topic}</p>
    <p><strong>Message :</strong><br>{message}</p>
    <hr>
    <p>Fichier enregistré : <code>{filename}</code></p>
    <p>Date : {now.strftime('%Y-%m-%d %H:%M:%S')}</p>
</body>
</html>""")
