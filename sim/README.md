The default build in this directory produces `sim_axi`, which instantiates
the AXI4-Lite SoC. The repository-level `make all` runs its hardware and
firmware regressions.

Use `make -C sim top=0` from the repository root to build the split-RAM
simulator (`sim`), or `make -C sim top=1` to build the single-RAM simulator
(`sim_single`). The repository-level `make hello` runs the split-RAM
diagnostic with RTL/ISS trace comparison.
