import os
import subprocess
from pathlib import Path
from datetime import datetime

# Provide a stub for Sphinx 'tags' in non-Sphinx contexts (linters/tests)
_tags_obj = globals().get("tags", None)
if _tags_obj is None:
    class _Tags:
        def add(self, *_args, **_kwargs):
            pass
    tags = _Tags()

# -- Project information -----------------------------------------------------

project = "CAN Lecture RX/TX"
author = "Project Contributors"
current_year = datetime.now().year
copyright = f"{current_year}, {author}"

# -- General configuration ---------------------------------------------------

extensions = [
    "myst_parser",
    "sphinx.ext.autosectionlabel",
    "breathe",
]

# MyST configuration to support GitHub-flavored Markdown conveniences
myst_enable_extensions = [
    "colon_fence",
    "deflist",
    "html_image",
    "attrs_block",
]

# Enable eval-rst for embedding Breathe directives
myst_fence_as_directive = ["eval-rst"]

templates_path = ["_templates"]
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
]

# -- Options for HTML output -------------------------------------------------

html_theme = "furo"
html_title = project
html_static_path = ["_static"]

# Make section labels unique across files
autosectionlabel_prefix_document = True

# --- Breathe / Doxygen integration ------------------------------------------

_DOCS_DIR = Path(__file__).parent
_DOXY_DIR = _DOCS_DIR / "_doxygen"
_DOXY_XML = _DOXY_DIR / "xml"

def _maybe_run_doxygen():
    """Run Doxygen if it's available and XML output is missing or forced.

    This makes local and CI builds resilient: if Doxygen isn't installed,
    the Sphinx build still succeeds (API page will show a note instead).
    """
    doxyfile = _DOCS_DIR / "Doxyfile.generated"
    if not doxyfile.exists():
        return
    force = os.environ.get("DOXYGEN_FORCE", "0") == "1"
    if _DOXY_XML.exists() and not force:
        return
    exe = os.environ.get("DOXYGEN", "doxygen")
    try:
        subprocess.run([exe, str(doxyfile)], check=True, cwd=_DOCS_DIR)
    except Exception as exc:
        # Soft-fail: leave a breadcrumb for debugging without breaking docs
        print(f"[docs] Skipping Doxygen: {exc}")

_maybe_run_doxygen()

if _DOXY_XML.exists():
    breathe_projects = {
        "CAN_Lecture": str(_DOXY_XML),
    }
    breathe_default_project = "CAN_Lecture"
    breathe_show_include = False
    breathe_show_enumvalue_initializer = True
    breathe_default_members = ('members', 'undoc-members')
    # Expose a tag so pages can conditionally include Breathe directives
    try:
        tags.add("with_doxygen")  # type: ignore[name-defined]
    except Exception:
        pass
else:
    # Provide empty config so Sphinx doesn't error if 'breathe' is present
    breathe_projects = {}
    try:
        tags.add("without_doxygen")  # type: ignore[name-defined]
    except Exception:
        pass
