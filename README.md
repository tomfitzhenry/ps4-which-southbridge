# ps4-which-southbridge

A tiny, pure-userland payload that reports a jailbroken PS4's southbridge
(Aeolia, Belize, Baikal, Belize2, ...).

## Why

Booting Linux on a PS4 requires knowing its southbridge: the kernel and device
trees differ per southbridge.

- **GoldHEN** shows the southbridge in System Information,
  but **ps4-hen** does not.
- The older "which southbridge" payloads run in kernel context with hardcoded
  firmware offsets, so a firmware mismatch jumps to a wrong kernel address and
  panics the console.

This tool reads the ID from userland instead, so it is safe on any firmware.

## How

It resolves `sysctlbyname` from `libkernel.sprx` via the `dynlib` syscalls
(594/591) and calls:

```
sysctlbyname("hw.sce_subsys_subid", &id, &len, NULL, 0)
```

It runs no kernel code and uses no kernel symbol offsets.

## Output

Reported two ways, so at least one is visible:

- a system notification,
- the klog (`write(1, ...)`).

Example: `PS4 Southbridge: Belize2 A0 (0x40100)`

| ID | Southbridge |
|---|---|
| `0x10100` / `0x10200` / `0x10300` | Aeolia A0 / A1 / A2 |
| `0x20100` / `0x20200` | Belize A0 / B0 |
| `0x30100` / `0x30200` / `0x30201` | Baikal A0 / B0 / B1 |
| `0x40100` | Belize2 A0 |

## Build

Only `gcc` and `objcopy` are needed (no Sony SDK, no yasm):

```console
$ make          # produces which-southbridge.bin
```

With Nix: `nix-shell --run make`.

## Run

Copy `which-southbridge.bin` to `/data/payloads/` on the PS4 (or a USB
`/payloads/`), then launch it from **Payload Guest** (Al-Azif).
It must run after HEN.

## References

- `hw.sce_subsys_subid` and the ID table come from ps4-linux-loader's AIO
  payload, `get_sb_id()` / `GetSouthbridgeName()`:
  <https://github.com/ps4-linux/ps4-linux-loader/blob/master/linux/main-aio.c>
- The `dynlib` syscalls (594 `dynlib_load_prx`, 591 `dynlib_dlsym`) and the
  freestanding build recipe:
  <https://github.com/ps4-linux/ps4-linux-loader/tree/master/lib>
- The same IDs appear in the FW-9.00-only kernel payload `whos-that-southbridge`
  (<https://github.com/upal212/Payload-Guest-With-Icons>), the kind of
  firmware-specific payload this tool avoids.
- Southbridge to model table: <https://github.com/feeRnt/ps4-linux-12xx>
- GoldHEN added Southbridge info in v2.2.3:
  <https://github.com/GoldHEN/GoldHEN/blob/master/CHANGELOG.md>
- ps4-hen has no southbridge info:
  <https://github.com/Scene-Collective/ps4-hen>
