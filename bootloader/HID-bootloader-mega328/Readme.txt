This is official HID bootloader from v-usb modified for the usbXR board with ATmega328

Steps for flashing a fresh board:
=================================

1. Connect an ISP programmer to the 6 ISP pins of usbXR board. 
Make sure to provide only 3.3v target voltage from the programmer. 
5v would damage the RFM70 transceiver. If any ISP programmer
other than USBasp is used, change the Makefile accordingly.

2. From the command line, execute these commands:

    1) make clean
    2) make all
    3) make fuse
    4) make program
