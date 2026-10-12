# Starbie

Starbie is a custom embedded electronics project built around the Seeed Studio XIAO ESP32-C3 (RISC-V). It combines a Seeed XIAO ESP32-C3, a DHT11 temperature and humidity sensor, an MPU6050 IMU, a 0.96-inch I2C OLED display, and two push buttons on a custom two-layer PCB designed from scratch in KiCad.

## Hardware Components

* **Microcontroller:** [Seeed Studio XIAO ESP32-C3](https://wiki.seeedstudio.com/SeeedStudio_XIAO_ESP32C3/) (RISC-V 32-bit RISC-V single-core CPU, Wi-Fi, BLE)
* **Display:** 0.96-inch I2C Monochrome OLED Display (128x64 resolution, SSD1306 driver)
* **Temperature/Humidity Sensor:** [DHT11 Sensor](https://www.component101.com/sensors/dht11-temperature-and-humidity-sensor) with a $10\text{ k}\Omega$ pull-up resistor on the data line
* **Motion Sensor:** MPU6050 6-axis Accelerometer & Gyroscope
* **User Input:** 2x Tactile push buttons
* **Expansion:** 1x4 and 1x8 pin headers

## Repository Structure

```
starbie/
├── Firmware/
│   ├── include/
│   ├── lib/
│   ├── src/
│   │   └── main.cpp          # Core C++ firmware
│   └── platformio.ini        # PlatformIO build configuration
├── Hardware/
│   ├── startbie.kicad_sch    # Schematic capture file
│   ├── startbie.kicad_pcb    # PCB layout file
│   ├── startbie.kicad_pro    # KiCad project file
│   └── Gerbers/              # Manufacturing files
└── README.md
```

## Firmware & Toolchain

The firmware is written in C++ using the Arduino framework via PlatformIO in VS Code. It handles I2C communication with the OLED display while polling the push buttons and reading sensor data.

### Dependencies
* [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)
* [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)

![VS Code terminal showing successful PlatformIO build](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/3d636cfbbc034a9d2b5e3016ba48104b042ad44ee69c4767e7142e852985cf53.jpg)

---

## 10-Hour Build Log

### Hour 1: Environment Setup and WinError 10013
I started by setting up a PlatformIO workspace for the ESP32-C3, referencing the [Seeed Studio XIAO ESP32-C3 Wiki](https://wiki.seeedstudio.com/SeeedStudio_XIAO_ESP32C3/). Right away, the build crashed with `WinError 10013: An attempt was made to access a socket in a way forbidden by its access permissions`. 

After trying admin mode and checking firewalls with no luck, I dug through [PlatformIO Community Forum Discussions and GitHub Issues](https://community.platformio.org/) and realized local network security was blocking the registry port. Changing the package mirror source and clearing the local cache fixed the socket block and let dependencies download.

### Hours 2–4: Firmware Architecture
I wrote the core application logic in `Firmware/src/main.cpp`, integrating the [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306) and [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library). Compiling for the RISC-V architecture taught me a lot about managing memory and library conflicts before successfully generating the binary image.

### Hours 5–7: Schematic Design
Moving to hardware, I opened KiCad (`startbie.kicad_sch`) and placed the XIAO ESP32-C3 (`U2`), DHT11 (`U1`), push buttons (`SW1`, `SW2`), pin headers (`J1`, `J2`), and the $10\text{ k}\Omega$ pull-up resistor (`R1`) based on the [DHT11 Datasheet](https://www.component101.com/sensors/dht11-temperature-and-humidity-sensor).

![Initial schematic layout of Starbie components](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/3672f16fc48ff644601823a9ec767ccc451dc171324d1bc1e148893a48449522.jpg)

I learned why pull-up resistors are needed for digital single-wire data lines and how power flags (`+3.3V`, `+5V`, `GND`) work.

![KiCad Tools menu for updating PCB and assigning footprints](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/da4be0d8d50c84cd626e73e57ae627b671ddc91f855aa5702d042832971d28c9.jpg)

Running the Electrical Rules Checker (ERC) helped catch floating pins before moving to the board layout.

![Completed KiCad circuit schematic capture](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/47fa95a2a3d84a4fe9878d8121d20500f09f5553a38c8bc44c46ff07feabeb41.jpg)

### Hours 8–9.5: Footprints, Layout, and DRC
Updating the PCB layout from the schematic (referencing the [KiCad Getting Started Guide](https://docs.kicad.org/)) threw missing-footprint errors because default symbols weren't bound to physical packages.

![KiCad global footprint libraries table configuration](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/3686c80bce90e74715f1395930beffae8d3005328ae821fc30dc73efeba2c8d7.jpg)

I used the Assign Footprints tool to map symbols to their exact physical packages.

![Assign Footprints tool overview](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/98fa1c127106978939af6c9be489014fdf76c38183edb0a0bef83dcd1aa22290.jpg)

I hit a few search misses at first...

![Assign Footprints search showing zero matches for missing package](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/902f9ec7cd9dd84ce0b0a9ff401a2ca423cacc8ca5c56e8237a91ba6c8e3fab0.jpg)

...but after fixing library paths, I found the correct packages (`Sensor:Aosong_DHT11` and `Seeed_Studio_XIAO_Series:XIAO-ESP32-C3-SMD`).

![Assign Footprints tool successfully locating the Seeed ESP32-C3 package](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/a15d3208b8c11ff1be92c105596e0a1038b3614e82a3df06ad258508b8edf5d8.jpg)

Then I initialized the PCB canvas in `startbie.kicad_pcb`...

![Initial PCB editor workspace setup](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/ab9061ba43f7199438c0b0083fa64053d7e4b150088fa53455911217a6d3a5f7.jpg)

...placed the components...

![PCB Editor component placement overview](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/31865ac90f2c0b4742c99b63033580c00afa6e00a36eebc5ab6cedb217668843.jpg)

...and routed the copper traces on the top (`F.Cu`, red) and bottom (`B.Cu`, blue) layers.

![PCB Editor showing routed copper tracks](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/990ef104896fb866b8a69dd0e3cefea022af8e1c2555ded6b05a86dbf7bee0b0.jpg)

When I ran the Design Rules Checker (DRC), it flagged 5 errors and 20 warnings.

![Design Rules Checker window showing courtyard and track width errors](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/f340d638d51daad1920c314e6d9f1426e89be20078605acced034edde3372e3d.jpg)

I had to fix courtyard errors, widen traces that were under $0.2000\text{ mm}$, and clear silkscreen text overlapping solder masks.

![PCB zoom-in showing silkscreen and solder mask warnings](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/33e951805220f0a420a893caa80c2b21075a0aa14e1073bbfa2119410bb1eb6c.jpg)

### Hour 10: Gerbers and GitHub
With a clean DRC check, I generated manufacturing files and inspected them in the KiCad Gerber Viewer.

![KiCad Gerber Viewer verifying final production layers](https://halflife.hackclub-assets.com/hackclub-half-life/sessions/OcYWjcDouGEzidswYkZNmkY7iSde7Zal/88f93e064083f98b4c3033d99ed11282fe4db9e66314b90c788f7326aaa1122b.jpg)

Finally, I added a `.gitignore`, initialized git, and pushed the repo to GitHub.

---

## References

* [Seeed Studio XIAO ESP32-C3 Wiki](https://wiki.seeedstudio.com/SeeedStudio_XIAO_ESP32C3/)
* [PlatformIO Community Forums](https://community.platformio.org/)
* [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)
* [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
* [DHT11 Sensor Datasheet](https://www.component101.com/sensors/dht11-temperature-and-humidity-sensor)
* [KiCad Documentation](https://docs.kicad.org/)
