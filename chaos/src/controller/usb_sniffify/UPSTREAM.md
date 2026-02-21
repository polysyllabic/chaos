# Vendored From `usb-sniffify`

- Upstream repository: `https://github.com/polysyllabic/usb-sniffify.git`
- Upstream commit: `90213252fa8ed6bfb0d2d1b67b4a78fe35c65e69`
- Imported into Chaos: `chaos/src/controller/usb_sniffify`

## Imported Files

- `include/raw-gadget.hpp`
- `include/raw-helper.h`
- `src/raw-helper.cpp`
- `src/raw-gadget.cpp`
- `src/raw-gadget-passthrough.cpp`
- `LICENSE`

## Local Patches

- Added deterministic passthrough shutdown by joining background threads in
  `RawGadgetPassthrough::stop()`.
