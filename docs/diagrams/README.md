# Diagrams

This folder contains PlantUML sources describing the project architecture. Render them with any PlantUML renderer or VS Code extension.

Files:
- architecture_component.puml — Component/flow overview of RX runtime
- rx_class_diagram.puml — Classes and relationships for RX modules
- can_sequence_rx.puml — Sequence of a received CAN frame to UI/IO updates
- rx_state_machine.puml — High-level system state machine
- tx_sequence_send.puml — TX board sending flow

Rendering (optional):
- VS Code: open a .puml file and use a PlantUML preview/render extension.
- CLI: `plantuml -tsvg *.puml` to generate SVGs.

**Important for documentation:**
After generating diagrams, you must copy them to the Sphinx `_static` directory:
```bash
# From the docs directory:
./copy_diagrams.sh      # On Linux/Mac
.\copy_diagrams.ps1     # On Windows
```

Then rebuild the Sphinx documentation:
```bash
cd docs
sphinx-build -b html . _build/html
```

Notes:
- Diagrams reflect headers under `src/` as of the time of generation.
- The .puml sources are ignored by Git per request; rendered SVG outputs in `_static/diagrams` should be committed for GitHub Pages.
- Keep this README tracked.
