# Zephyr BLE Mesh - Light

Here you can find source code for a light node inside Bluetooth Mesh, that waits for message from switch to turn on or off LED0 after it is provisioned.
## Structure

    ├── src                    
    │   ├── main.c      # Source file
    ├── CMakeLists.txt  # For building with cmake
    ├── prj.conf        # Configuration file 
    └── README.md
## Prerequisites
### Hardware
During development Nordic's [nRF52840](https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk) board was used, but code should run on any with Zephyr RTOS and BLE support. For transferring binaries to the boards and their powering micro USB cables are necessary.
### Software
Zephry RTOS & alongside its SDK should be installed on the machine where application build will be done, during implementation version 1.14 was used and should work on Windows, MacOS or Linux environment. 
[nRF Mesh](https://www.nordicsemi.com/Products/Development-tools/nRF-Mesh) from Nordic Semiconductor was used for Bluetooth mesh provisioning and configuration.
If your board has LED on different PIN, change row 5 accordingly.

For building Zephyr's meta-tool West is used and it requires two environmental variables set:

* ZEPHYR_SDK_INSTALL_DIR=<path_to_zephyr_sdk>
* ZEPHYR_TOOLCHAIN_VARIANT=zephyr

```bash
#Command for building binary. It should be run from zephyrproject/zephyr folder
west build -p auto -b nrf52840_pca10056 zephyr-ble-mesh/light

#Command for flashing binary on connected board
west flash
```
#### Troubleshooting
```Error: could not find CMAKE_PROJECT_NAME in Cache ```: Delete CMakeCache.txt file from zephyrproject/zephyr/build


## Author
- [Jaka Purg](https://www.linkedin.com/in/jaka-purg-9b25551a6/)