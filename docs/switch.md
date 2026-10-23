# Nintendo Switch

A port for homebrew (`atomic.nro`), built by `.github/workflows/switch.yml` with devkitA64 and libnx.
The workflow keeps the finished package as the artifact `atomic-bomberman-modern-switch`.

**State of things:** it builds. It has not been run on a console yet, so the first run is a test:
the file `log.txt` (see below) tells what happened.

## Installing

1. Unpack `atomic-bomberman-modern-switch.zip` onto the root of the SD card: it puts `atomic-bomberman.nro` in `/switch/atomic-bomberman/`. (A build from Actions artifacts has the same zip.)
2. Start it from the Homebrew Menu. Start it by taking over a game's title (hold R when opening a
   game), not through the Album: the Album gives homebrew very little memory.
   Or send it from a computer on the same network with nxlink (Homebrew Menu: press **Y** to wait
   for it): `nxlink -a CONSOLE_IP -s atomic.nro`. With `-s` the log of the game appears in the
   terminal as it is written, instead of in `log.txt`.
3. The game makes its folder on the SD card, `/switch/atomic-bomberman/`:

| Path | What |
|---|---|
| `original/` | Copy your own copy of Atomic Bomberman (1997) here: the folder with `COLOR.PAL` and `DATA`. At the next start it is converted into `assets/`. Without it the game plays with its free graphics and sounds. |
| `assets/` | The converted data. Delete it to have it made again. |
| `mods/` | Files of your own, in the layout of the data folder (see the `README.txt` the game puts there). Copied over the data at every start. |
| `options.ini`, `schemes/` | Settings and levels you made. |
| `log.txt` | What the game wrote while it ran, including every controller found and its first button presses. Look here first when something is wrong. |

## Controllers

Up to eight controllers. A Joy-Con pair is one controller; to play with the halves separately,
press **−** on a pair (or **−** / **+** on a half) to open the system's controller screen
(*Change Grip/Order*) and split the pair. A Joy-Con held sideways on its own has its four
buttons as the action buttons and its stick as the arrows (the same mapping devkitPro's SDL2
uses); a Pro Controller, a pair and the console's own buttons work as usual. The stick press of a
single Joy-Con is Start.

## Online

Games by address and on the local network work as on a computer. Games found through a master
server need the keys `online_config_url` and `online_config_key` in `options.ini` (see
`docs/operations/online.md`).

## How it is built

* SDL3 is not available on the Switch; `src/platform/switch/` stands in for the SDL3 header
  (`sdl3_on_sdl2.hpp`) on top of devkitPro's SDL2 (window, OpenGL ES, touch) and libnx
  (controllers: `HidNpad`, the controller screen). Sound is mixed by a small mixer there.
* mbedTLS gets its random numbers from libnx's `csrng` (`src/online/switch_entropy.c`); libjuice
  gets the console's address and `pipe()` from `src/platform/switch/ifaddrs/`.
* Locally: `source /opt/devkitpro/switchvars.sh && cmake -S . -B build-switch
  -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake && cmake --build build-switch`
  (needs `dkp-pacman -S switch-sdl2 switch-mesa switch-glad`).
