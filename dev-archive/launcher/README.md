# SH2VR launcher

One program for the Silent Hill 2 VR mod: installs or updates it, puts the game's controls back to their defaults,
starts the game through Steam and loads UEVR into it. `uninstall` takes everything out again, exactly as it was.

Build: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64` then `cmake --build build --config Release`.

Layout beside `SH2VR.exe`: `package\profile\` (the UEVR profile), `package\paks\` (pak mods), `uevr\` (UEVR's own files).

Commands: `play` (default), `attach`, `install`, `uninstall`, `status`. Options: `--game <folder>`, `--uevr <folder>`,
`--sim` (the OpenXR Simulator), `--runtime-json <file>`, `--no-install` (a test run with a freshly built plugin), `--openvr`, `--nullify`, `--delay <ms>`, `--test-root <folder>` (every destination goes into that folder; nothing real
is touched and nothing is launched).

The injection steps follow praydog's [uevr-frontend](https://github.com/praydog/uevr-frontend) (MIT). Notes and the
full list of what it places: `modding-notes/2026-11-05-uevr-inside-our-mod-and-the-launcher.md`.
