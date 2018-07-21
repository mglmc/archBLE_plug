# ArchBLE_Plug
This project is a smart plug based on BLE (Bluetooth Low Energy Wireless Technology) development in an Arch BLE board

The smart plug has several **functions**:

* **Remote control**: There are 2 characteristics. One to read the port and the other one to write into the relay port.   
* **Current AC monitoring**: There is a characteristic with PROPERTIES_NOTIFY. The way to read the AC current follow this web site [Tutorial sensor de corriente ACS712](https://naylampmechatronics.com/blog/48_tutorial-sensor-de-corriente-acs712.html).

The code is has been development with the [mbed online compiler](https://os.mbed.com/compiler/)
