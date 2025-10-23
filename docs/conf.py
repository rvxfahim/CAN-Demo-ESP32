import os
from datetime import datetime

# -- Project information -----------------------------------------------------

project = "CAN Lecture RX/TX"
author = "Project Contributors"
current_year = datetime.now().year
copyright = f"{current_year}, {author}"

# -- General configuration ---------------------------------------------------

extensions = [
    "myst_parser",
    "sphinx.ext.autosectionlabel",
]

# MyST configuration to support GitHub-flavored Markdown conveniences
myst_enable_extensions = [
    "colon_fence",
    "deflist",
    "html_image",
    "attrs_block",
]

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
