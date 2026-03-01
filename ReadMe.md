# Macropad - 6-Key Programmable Macro Pad with Scroll Wheel

![Macropad](images/macropad.jpg)  
*Replace with an actual image if available.*

A compact, fully customizable 6‑key macropad with a rotary encoder wheel. Designed to boost productivity, enhance gaming, or streamline any workflow that needs extra programmable inputs. Each key can trigger complex macros, toggle auto‑repeat, and even simulate up to six simultaneous keypresses. The wheel supports three configurable modes—directional keys, vertical scroll, or horizontal scroll—all switchable on the fly.

## Features

- **6 mechanical keys** – fully programmable.
- **Rotary encoder wheel** with three modes:
  - **Direction keys** – sends two keypresses (e.g., Left/Right or Up/Down) depending on rotation.
  - **Vertical scroll** – emulates mouse wheel scrolling.
  - **Horizontal scroll** – emulates horizontal scrolling (useful for timelines or spreadsheets).
- **On‑device profile storage** – (microSD card required) create and switch between multiple profiles saved in non‑volatile memory.
- **Flexible key behavior**:
  - **Short press** – sends a configured key combination.
  - **Toggle mode** – when toggled on, the key repeatedly sends its assigned macro with a user‑defined delay (spam mode).
- **Multi‑key emulation** – each physical key can trigger up to **6 virtual keypresses** simultaneously (e.g., `Ctrl+Alt+Delete`). The device follows the USB HID limit of 6 concurrent keys; if one key emulates 5 keys, only one other key can be pressed at the same time without exceeding the limit.
- **Sleep timer** – device enters sleep after 2 minutes of inactivity. Turning the wheel still delivers input; pressing a key wakes it up.

## USB HID Features

- **Full 224‑key support** – includes keys up to F24.
- **Media (Consumer) page support** – adjust volume, play/pause, skip tracks, etc.
- **Fast response** – response speed is primarily limited by the operating system; the firmware is faster than human reaction time.

## Getting Started

This project is derived from [Hapticpad](https://github.com/DDecoene/Hapticpad/). I am building upon it with both hardware and software improvements. For the original case and PCB gerber files, please refer to that repository.

### Hardware Changes

1. Added a **1N4007 diode** between VCC (voltage) of the controller board and the display.  
   *This is necessary for use with USB‑PD devices (e.g., MacBooks) that supply variable voltage (4.9–5.25 V). The diode steps down the voltage to stay within the display’s 3.3–5 V specification. MacBooks generally provide ~5.2 V, while many Windows PCs provide ~4.9 V.*
2. Switched switch footprints from Kailh to Keychron (Gateron) low‑profile switches (the same used in the K‑Max series). Case compatibility is preserved.

### Firmware Changes

- Limited force‑feedback on the wheel to a weakened “notch” mode, as the original felt unreliable.
- Redefined the extra buttons:  
  - Left button: triggers **BOOTSEL mode** (for firmware updates).  
  - Right button: cycles through **profiles**.

### Firmware Installation

1. Download the latest `.uf2` file from the [Releases](https://github.com/your-repo/releases) page.
2. Press the **BOOTSEL** button on your RP2040 board, then press **RESET**. Release RESET while holding BOOTSEL until the Pico appears as a mass storage drive.
3. Copy the downloaded `.uf2` file to that drive.
4. Format a microSD card as **FAT32** and place a `config.yaml` file (see `example/config.yaml`) in its root directory.

### Configuration Syntax

*Full documentation for the YAML configuration is coming soon. For now, refer to the `example/` folder for a sample setup.*

## Repository Structure

- `example/` – Contains a sample configuration file demonstrating all features.
- `firmware/` – Source code for the RP2040 firmware.
- `hardware/` – KiCad files and schematics (if any).
- `docs/` – Additional documentation (e.g., configuration guide).

## Planned Improvements

- Add support for better displays (e.g., E‑ink).
- Design a reinforced case with improved rigidity (current one flexes horribly).
- Enhance wheel force‑feedback for a more tactile feel.
- Improving original 'PCB' for better soldering experience

## Use Cases

- **Gaming** – rapid‑fire toggles, multi‑key combos.
- **Productivity** – custom shortcuts for IDEs, design tools, or office applications.

## Contributing

Contributions are welcome! Please open an issue or pull request for bug fixes, new features, or improvements.

## License

This project is licensed under the MIT License – see the [LICENSE](LICENSE) file for details.