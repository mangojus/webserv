#!/usr/bin/env python3
import sys, os
from urllib.parse import parse_qs
from utils import safe_path, GALLERY_PATH

# ── Logique normale ──
query = parse_qs(os.environ.get("QUERY_STRING", ""))
filename = query.get("filename", [""])[0]

if not filename:
    print("Status: 400 Bad Request")
    print("Content-Type: text/plain")
    print()
    print("Missing filename")
    sys.exit()

allowed = {'.jpg', '.jpeg', '.png', '.gif', '.webp'}
ext = os.path.splitext(filename)[1].lower()

if ext not in allowed:
    print("Status: 400 Bad Request")
    print("Content-Type: text/plain")
    print()
    print("Extension non autorisee: '{}'".format(ext))
    sys.exit()

path = safe_path(filename)
if path is None:
    print("Status: 400 Bad Request")
    print("Content-Type: text/plain")
    print()
    print("Nom de fichier invalide")
    sys.exit()

os.makedirs(GALLERY_PATH, exist_ok=True)

data = sys.stdin.buffer.read()

if len(data) == 0:
    print("Status: 400 Bad Request")
    print("Content-Type: text/plain")
    print()
    print("Fichier vide")
    sys.exit()

try:
    with open(path, "wb") as f:
        f.write(data)

    # ── Permissions 707 ──
    os.chmod(path, 0o707)

except Exception as e:
    print("Status: 500 Internal Server Error")
    print("Content-Type: text/plain")
    print()
    print("Erreur ecriture: {}".format(str(e)))
    sys.exit()

print("Status: 200 OK")
print("Content-Type: text/plain")
print()
print("Uploaded")