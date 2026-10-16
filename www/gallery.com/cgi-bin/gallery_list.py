#!/usr/bin/env python3
import json, os
from utils import GALLERY_PATH

files = []
if os.path.exists(GALLERY_PATH):
    files = sorted([
        f for f in os.listdir(GALLERY_PATH)
        if f.lower().endswith(('.jpg', '.jpeg', '.png', '.gif', '.webp'))
    ])

print("Status: 200 OK")
print("Content-Type: application/json")
print()
print(json.dumps({"images": files}))