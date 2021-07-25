# Zephyr BLE Mesh

This is a part of my bachelor's degree paper with a title Bluetooth Mesh application on [Zephyr operating system](https://docs.zephyrproject.org/latest/). For demonstrating purposes I implemented simple switch & light applications provisioned in a mesh by an iOS or Android app [nRF Mesh](https://www.nordicsemi.com/Products/Development-tools/nRF-Mesh).

## Structure

    
    ├── switch                   # Code & documentation for switch that controlls lights on other connected boards
    ├── light                    # Code & documentation for lights that wait for messages from the switch
    ├── tools                    # Tools for easier debugging & logging
    └── README.md
## Prerequisites
### Hardware
The coding was done on a Nordic's [nRF52840](https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk) board, but will work on any board with Zephyr RTOS and Bluetooth Low energy support. For transferring binaries to the boards and their powering micro USB cables are necessary.
### Software
Zephry RTOS & alongside its SDK should be installed on the machine where application build will be done, during implementation version 1.14 was used and should work on Windows, MacOS or Linux environment. 
NRF mesh app from Nordic Semiconductor was used for Bluetooth mesh provisioning and configuration.
## Author
- [Jaka Purg](https://www.linkedin.com/in/jaka-purg-9b25551a6/)