/*
 * 	Common definitions for bootloader protocol
 *	
 */

 
/* Payload types */
#define PAYLD_TYPE_DEVINFO		0xa1
#define PAYLD_TYPE_STATUS		0xa2
#define PAYLD_TYPE_COMMAND		0xa3
#define PAYLD_TYPE_DATA			0xa4

/* Commands */
#define OTA_PROG_START			0xaa
#define OTA_PROG_BOOT			0xab
#define OTA_PROG_STOP			0xac

/* Status */
#define OTA_PROG_FINISHED		0xbb