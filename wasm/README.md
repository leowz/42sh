# 42sh WebAssembly Build

Run 42sh in the browser via a full Linux VM compiled to WebAssembly using [container2wasm](https://github.com/container2wasm/container2wasm).

## Prerequisites

- Docker
- curl (for downloading c2w)
- ~2GB disk space for the build

## Build

```bash
cd wasm
chmod +x build.sh
./build.sh
```

This will:
1. Download the `c2w` converter (if not already present)
2. Build a Docker image containing 42sh
3. Convert it to browser-ready Wasm + JS files in `wasm/dist/`

## Run locally

```bash
cd wasm/dist
python3 -m http.server 8080
```

Open http://localhost:8080 — you'll get an xterm.js terminal running 42sh.

## Deploy to GitHub Pages

Copy the contents of `wasm/dist/` to your GitHub Pages root or a `docs/` folder.

Note: GitHub Pages requires the `Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` headers for SharedArrayBuffer. The generated files include a `coi-serviceworker.js` that handles this automatically.

## How it works

container2wasm packages your compiled 42sh binary inside a minimal Debian environment, then emulates the entire Linux kernel + userspace via a CPU emulator (TinyEMU/Bochs) compiled to WebAssembly. The browser runs xterm.js as the terminal frontend, connected to the emulated serial console.
