---
title: C/C++ API Reference
---

# C/C++ API Reference

This section is generated from the source code using Doxygen + Breathe.

If this page appears empty in local builds, ensure Doxygen is installed and re-run the docs build. In CI, we attempt to run Doxygen automatically when available.

```{only} with_doxygen
```{eval-rst}
.. doxygenindex::
   :project: CAN_Lecture
```
```

```{only} without_doxygen
Note: Doxygen XML was not found during this build. The API index is hidden. Install Doxygen and rebuild, or enable it in CI.
```

## Quick links

- Message routing: `src/common/MessageRouter.h`
- RX orchestration: `src/rx/SystemController.h`
- Event queue: `src/rx/EventQueue.h`
- CAN interface: `src/rx/CanInterface.h`
- IO outputs: `src/rx/IoModule.h`
- UI controller: `src/rx/UiController.h`
