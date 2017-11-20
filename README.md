usbXR - Low cost USB Wireless transceiver
=========================================

usbXR is basically a USB adapter for the RFM75 2.4GHz wireless module. The USB functionality is implemented using AVR ATmega328 and the [v-usb](https://www.obdev.at/products/vusb/index.html) firmware-only USB driver for AVRs, which emulates USB low-speed protocol. The device can be used for bi-directional communication with *remote* nodes, containing the same wireless module (ie. RFM75 or other compatible modules like RFM70/RFM73).

Once the [HID-Bootloader] is flashed, you can load your application to usbXR using a command-line utility. In addition you can program remote node AVRs over-the-air! Both remote and self programming uses a single command line application bootloadHID:

	**bootloadHID.exe receiver.hex** - programs usbXR device with _receiver.hex_
	**bootloadHID.exe remote transmitter.hex** - programs a remote node with _transmitter.hex_
	
For over-the-air programming, the remote AVR need to be initially programmed with a bootloader. An example for ATmega8 is given [here].
	
[applications] directory contains example applications.

 
Hardware
--------

The hardware is simple and can only cost as low as 5 USD to make. [hardware](https://github.com/visakhanc/usbXR/tree/master/hardware) directory contains the eagle schematic and layout with Partlist.
You can order the PCBs from [OSHpark](https://oshpark.com/shared_projects/8Y8Vg5b4).


Setup
-----

HID bootloader need to be programmed to the AVR to make use of self and remote programming over USB. The bootloader and the command-line utility is an extension to the official HID [bootloader] from v-usb, modified to incorporate remote bootloading functionality. 

**IMPORTANT:** Before building any AVR projects from this repository, common library need to be downloaded first. This contains many device libraries for AVRs such as RFM70 etc. Download it and rename the directory to 'common'. Downloaded repositories must be at same directory level for Makefile to work. That is:

	AVR_Projects
	|
	|-- common
	|
	|-- usbXR

	
### Bootloader

For building the bootloader firmware under Windows, WinAVR is needed. To flash the AVR, after the hardware is ready, connect an ISP programmer like USBasp to the 6 ISP pins of usbXR. Make sure to provide only 3.3v target voltage from the programmer. 5v would damage the RFM70 transceiver. To compile and program the HID-Bootloader, go into the bootloader directory and execute these steps:

1. make clean
2. make all
3. make fuse
4. make program

*If any ISP programmer other than USBasp is used, change the Makefile accordingly.*

Now, plug usbXR into a USB port and it will be recognized as an HID device (HIDBoot).


### Bootloader utility

You will need MinGW installation to compile under Windows. To compile BootloadHID application under Windows, execute:

	make -f Makefile.windows
	
To use the bootloader, plug-in the device to a USB port while pressing down the button. The LED will be on and it is now in bootloader mode. Now use the BootloadHID application as explained earlier, to self or remote programming.


### Remote bootloader

If you want to use the over-the-air programming feature of usbXR, a bootloader need to be initially programmed to the AVR. The example given is for ATmega8, but can be used for other AVRs with at least 2kB of boot space. The bootloader uses the last byte of the AVR EEPROM to store a validity flag. View the readme for building instructions. A button and LED is expected for a remote device. To enter bootloader, the button needs to be pressed while powering-on or resetting the AVR. Now, the bootloadHID tool can be used for programming. The LED flashes at 1 sec interval, when over-the-air programming is in progress until the programming is over. If programming fails midway, the command needs to be repeated. Programming is successful only when the LED stops flashing.


Applications
------------

An [example] shows a remote node sending 3 axis accelerometer values from MPU6050 sensor, continuously to usbXR and plotting the values in real-time. Here, interrupt-in endpoint is used to transfer the received samples over USB, at intervals as short as 10ms. Since it uses HID protocol, no driver is required under Windows. A WPF C# application is used to get USB packets and draw the graph.

Please visit [v-usb](https://www.obdev.at/products/vusb/index.html) for more details of implementating USB devices using v-usb library. For easy access of USB device, PyUSB python library can be used.