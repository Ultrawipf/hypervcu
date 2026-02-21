## Hyper VCU

**INFO: This project is in early development. Many features are not yet fully implemented or tuned**

### Application
DIY replacement vehicle controller for Joyor T6E scooters for use with VESC motor drivers and the original controls.
Currently only supports Joyor T6E.

### Features

* Controls all lights: Front, Rear, Brake, Indicators
* 3 Drive modes + 3 additional "master" drive modes
    * Speed, Current and control mode (Throttle vs. Speed control) for every mode selectable
* Configurable Off-throttle and Brake regen strength
* R503 Fingerprint reader support
    * Locks scooter until valid fingerprint is scanned
    * Enables secondary drive modes with "master finger"
* USB & OTA firmware update
* VESC Control via UART
* Reads charge level from original BMS via BLE
* Configuration via WiFi webinterface (ESPUI based)
    * Connects to existing AP or fallback to standalonge AP mode (Password + Hidden SSID supported)

### Connections

TODO


### Screenshots

![Drive page](doc/img/drivepage.png)
![Mode page](doc/img/modepage.png)
![System page](doc/img/syspage.png)