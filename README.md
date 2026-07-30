# Gargantua Studio

Gargantua Studio is an offline Kerr black-hole film renderer powered and
validated by the independent Solar physics library.

This repository currently contains the first cross-repository CPU probe. It
does not yet contain a renderer, CUDA backend, radiation model, or claim of
cinema-quality output.

## Build against the locked local Solar checkout

The Solar checkout must be at the exact commit recorded in
`cmake/solar-lock.cmake`:

```sh
cmake -S . -B build \
  -DGARGANTUA_SOLAR_SOURCE_DIR=/path/to/solar-at-the-locked-commit
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/gargantua-probe
```

Omit `GARGANTUA_SOLAR_SOURCE_DIR` to fetch the same commit from the canonical
Solar repository after that commit has been published. Generated artifacts
must record the Solar version and physics contract.
