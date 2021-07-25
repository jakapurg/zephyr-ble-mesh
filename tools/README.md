# Zephyr BLE Mesh - Tools

Here you can find bash script for Real-Time Tracing logging from our boards to standard output. If you use any other board, you should change device parameter in script that runs JLinkRTTLogger installed with Segger J-Link Software. Zephyr's documentation for logging with RTT can be find [here](https://docs.zephyrproject.org/latest/guides/tools/nordic_segger.html#rtt-console).

## Structure

    
    ├── logging.sh          # Script for running RTT logger
    ├── logging.desktop     # Desktop shortcut for Linux for more efficient work 
    └── README.md
## Prerequisites
### Software
Script logging.sh uses JLinkRTTLogger binary that needs to be installed in /bin folder and can be download from Nordic's [webpage](https://www.nordicsemi.com/Products/Development-tools/nRF-Command-Line-Tools). If you use any other board than nRF52840, you should replace -Device parameter with your own board name.
## Author
- [Jaka Purg](https://www.linkedin.com/in/jaka-purg-9b25551a6/)