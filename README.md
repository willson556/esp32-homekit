# esp32-homekit

This project impements Apple Homekit Accessory Protocol(HAP) on the ESP32.\
You can make your own Homekit accessory with an ESP32 using this library.

## Demo
[![ESP32 HOMEKIT](https://img.youtube.com/vi/OTBtEQNa-1E/0.jpg)](https://www.youtube.com/watch?v=OTBtEQNa-1E "ESP32 HOMEKIT")

## Resources
- [Apple Homekit Accessory Protocol](https://developer.apple.com/support/homekit-accessory-protocol/)
- [Mongoose](https://github.com/cesanta/mongoose)

## Prerequisites
The `esp32-homekit` is using esp-idf libraries and build.\
Please install ESP-IDF (v3.1 recommended)
- ESP-IDF Setup Guide
  * [Windows Setup Guide](https://docs.espressif.com/projects/esp-idf/en/v3.1/get-started/windows-setup.html)
  * [Mac OS Setup Guide](https://docs.espressif.com/projects/esp-idf/en/v3.1/get-started/macos-setup.html)
  * [Linux Setup Guide](https://docs.espressif.com/projects/esp-idf/en/v3.1/get-started/linux-setup.html)

## Add to Existing Project

Add as a submodule in the `components` folder of your project:

```
$ git submodule add https://github.com/willson556/esp32-homekit.git components/homekit
$ git submodule update --init --recursive
```

## Run Examples

### Clone

```
$ git clone  https://github.com/willson556/esp32-homekit.git
$ cd esp32-homekit
$ git submodule update --init --recursive
```

### WiFi Configuration

esp32-homekit uses WiFi as tranmission layer.\
To connect to WiFi, you MUST configure the WiFi SSID and password.

1. Open examples/switch/main/main.c
2. Change EXAMPLE_ESP_WIFI_SSID, and EXAMPLE_ESP_WIFI_PASS

```c
#define EXAMPLE_ESP_WIFI_SSID "unibj"
#define EXAMPLE_ESP_WIFI_PASS "12345678"  
```

### Build

```bash
$ cd examples/switch
$ make
$ make flash
```

# Setup Code
While pairing accessory and iOS devices, You must enter Setup Code in the _Home_ App.
The default setup code is 
## **`053-58-197`**


