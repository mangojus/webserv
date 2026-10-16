import os

# Chemin absolu vers le dossier gallery, peu importe d'où le CGI est lancé
_HERE = os.path.dirname(os.path.abspath(__file__))
GALLERY_PATH = os.path.normpath(os.path.join(_HERE, "../html/gallery"))

def safe_path(filename):
    """Retourne un chemin sûr (pas de path traversal)."""
    if not filename:
        return None
    basename = os.path.basename(filename)
    if not basename:
        return None
    return os.path.join(GALLERY_PATH, basename)