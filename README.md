# recoil_gui_touchscreen
### Features
- Eliminates predefined recoil patterns
- Teensy 4.1 is both USB host (for mouse) and device (to computer)
- Works as a mouse passthrough when recoil compensation is not active
- Injected mouse movement is 'real' to the host
- GUI with touchscreen to adjust settings, built with LVGL

**USB Mouse -> Teensy -> Computer**
## Images
![Teensy 4.1 ILI9341](/Images/IMG_0301.JPEG)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;![Main Menu](/Images/IMG_0309.JPEG)

![Dropdown Menu](/Images/IMG_0311.JPEG)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;![Toggle Button](/Images/IMG_0315.JPEG)

![Arc Slider](/Images/IMG_0316.JPEG)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;![Keypad](/Images/IMG_0319.JPEG)
## How to build
### Hardware list
- [Teensy 4.1](https://www.pjrc.com/store/teensy41.html)
- [ILI9341 TFT Display](https://www.pjrc.com/store/display_ili9341_touch.html)
- USB type A female header
- ~100ohm  resistor
### Dependencies
- [Teensyduino IDE](https://www.pjrc.com/teensy/td_download.html)
- [ILI9341 Library](https://github.com/PaulStoffregen/ILI9341_t3)
- [XPT2406 Library](https://github.com/PaulStoffregen/XPT2046_Touchscreen)
- [USBHost_t36 Library](https://github.com/PaulStoffregen/USBHost_t36)
- [LVGL Library v8.0](https://github.com/lvgl/lvgl)
## TODO
- Add randomization feature in recoil class and implement a slider for adjustment
- Add non volatile storage (SD card) to save settings and load on startup
- Add favorites buttons for equipment loadouts
- Adjust some recoil tables
- Add better support for semi automatic equipment in recoil class
