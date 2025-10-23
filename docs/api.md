# API Reference (C/C++)

This project is primarily C/C++ embedded code. Sphinx's built-in autodoc targets Python, so for rich API documentation we recommend integrating Doxygen with the Breathe Sphinx extension.

## Planned enhancement

We will add a Doxygen + Breathe pipeline to extract C/C++ symbols and render them here. Outline:

1. Add a `Doxyfile` targeting `src/` and `lib/`.
2. Generate XML output: `doxygen -g` then `doxygen`.
3. Add Breathe to `docs/requirements.txt` (`breathe`), and update `docs/conf.py`:
   - `extensions += ["breathe"]`
   - `breathe_projects = {"CAN-Lecture": "_doxygen/xml"}`
   - `breathe_default_project = "CAN-Lecture"`
4. Create reStructuredText or MyST pages with `breathe` directives (`doxygenindex`, `doxygennamespace`, `doxygenclass`, etc.).

Until then, refer to these source locations:

- CAN driver abstraction: `lib/CanDriver/`
- Message router core: `src/common/MessageRouter.{h,cpp}`
- RX CAN interface: `src/rx/CanInterface.{h,cpp}`
- DBC wrapper include: `src/generated_lecture_dbc.c`
