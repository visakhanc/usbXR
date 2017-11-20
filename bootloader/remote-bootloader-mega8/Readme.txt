This is the bootloader to be flashed to the AVR of the remote node, in order to program the device using usbXR. 

ATmega8 is used here. But this can be changed in the Makefile for an other AVR. The bootloader uses last byte of EEPROM memory to store the validity of application presense.

Steps to build and program the bootloader:

1. make clean
2. make all
3. make fuse
4. make program
	
	
