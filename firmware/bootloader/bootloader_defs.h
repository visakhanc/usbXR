/*
 * 	bootloader_defs.h
 *
 * 	Common definitions for remote boot protocol of usbXR
 *	
 */

#ifndef _BOOTLOADER_DEFS_
#define _BOOTLOADER_DEFS_



/* Commands */
#define CMD_OTA_BOOT_START			0xa0
#define CMD_OTA_BOOT_RESET			0xa1
#define CMD_OTA_BOOT_STOP			0xa2
#define CMD_OTA_BOOT_END        	0xa3
#define CMD_OTA_BOOT_TXMODE			0xa4
#define CMD_OTA_BOOT_UPDATE			0xa5

/* Status types */
#define STATUS_TYPE_BOOT			0xb0
#define STATUS_TYPE_DEVINFO			0xb1

/* Status */
#define STATUS_OTA_BOOT_REQ			0xc0
#define STATUS_OTA_BOOT_READY		0xc1
#define STATUS_OTA_BOOT_OK			0xc2


#endif
