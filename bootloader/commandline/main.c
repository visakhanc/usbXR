/* Name: main.c
 * Project: AVR bootloader HID
 * Author: Christian Starkjohann
 * Creation Date: 2007-03-19
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: Proprietary, free under certain conditions. See Documentation.
 * This Revision: $Id$
 *
 * Modified: 19-Nov-2017
 * By: Visakhan C
 * For: usbXR project: https://github.com/visakhanc/usbXR
 */



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <windows.h>
#include "usbcalls.h"
#include <stdbool.h>

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

#include "../bootloader_defs.h"

#define IDENT_VENDOR_NUM        0x16c0
#define IDENT_VENDOR_STRING     "obdev.at"
#define IDENT_PRODUCT_NUM       1503
#define IDENT_PRODUCT_STRING    "HIDBoot"

/* ------------------------------------------------------------------------- */

static char dataBuffer[65536 + 256];    /* buffer for file data */
static int  startAddress, endAddress;
static char leaveBootLoader = 0;

/* ------------------------------------------------------------------------- */

static void sleep_ms(int milliseconds) // cross-platform sleep function
{
#ifdef WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    usleep(milliseconds * 1000);
#endif
}


static int  parseUntilColon(FILE *fp)
{
int c;

    do{
        c = getc(fp);
    }while(c != ':' && c != EOF);
    return c;
}

static int  parseHex(FILE *fp, int numDigits)
{
int     i;
char    temp[9];

    for(i = 0; i < numDigits; i++)
        temp[i] = getc(fp);
    temp[i] = 0;
    return strtol(temp, NULL, 16);
}

/* ------------------------------------------------------------------------- */

static int  parseIntelHex(char *hexfile, char buffer[65536 + 256], int *startAddr, int *endAddr)
{
int     address, base, d, segment, i, lineLen, sum;
FILE    *input;

    input = fopen(hexfile, "r");
    if(input == NULL){
        fprintf(stderr, "error opening %s: %s\n", hexfile, strerror(errno));
        return 1;
    }
    while(parseUntilColon(input) == ':'){
        sum = 0;
        sum += lineLen = parseHex(input, 2);
        base = address = parseHex(input, 4);
        sum += address >> 8;
        sum += address;
        sum += segment = parseHex(input, 2);  /* segment value? */
        if(segment != 0)    /* ignore lines where this byte is not 0 */
            continue;
        for(i = 0; i < lineLen ; i++){
            d = parseHex(input, 2);
            buffer[address++] = d;
            sum += d;
        }
        sum += parseHex(input, 2);
        if((sum & 0xff) != 0){
            fprintf(stderr, "Warning: Checksum error between address 0x%x and 0x%x\n", base, address);
        }
        if(*startAddr > base)
            *startAddr = base;
        if(*endAddr < address)
            *endAddr = address;
    }
    fclose(input);
    return 0;
}

/* ------------------------------------------------------------------------- */

char    *usbErrorMessage(int errCode)
{
static char buffer[80];

    switch(errCode){
        case USB_ERROR_ACCESS:      return "Access to device denied";
        case USB_ERROR_NOTFOUND:    return "The specified device was not found";
        case USB_ERROR_BUSY:        return "The device is used by another application";
        case USB_ERROR_IO:          return "Communication error with device";
        default:
            sprintf(buffer, "Unknown USB error %d", errCode);
            return buffer;
    }
    return NULL;    /* not reached */
}

static int  getUsbInt(char *buffer, int numBytes)
{
int shift = 0, value = 0, i;

    for(i = 0; i < numBytes; i++){
        value |= ((int)*buffer & 0xff) << shift;
        shift += 8;
        buffer++;
    }
    return value;
}

static void setUsbInt(char *buffer, int value, int numBytes)
{
int i;

    for(i = 0; i < numBytes; i++){
        *buffer++ = value;
        value >>= 8;
    }
}

/* ------------------------------------------------------------------------- */

typedef struct deviceInfo{
    char    reportId;
    char    pageSize[2];
    char    flashSize[4];
}deviceInfo_t;

typedef struct deviceData{
    char    reportId;
    char    address[3];
    char    data[128];
}deviceData_t;

static int uploadData(char *dataBuffer, int startAddr, int endAddr)
{
usbDevice_t *dev = NULL;
int         err = 0, len, mask, pageSize, deviceSize;
union{
    char            bytes[1];
    deviceInfo_t    info;
    deviceData_t    data;
}           buffer;

    if((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM, IDENT_VENDOR_STRING, IDENT_PRODUCT_NUM, IDENT_PRODUCT_STRING, 1)) != 0){
        fprintf(stderr, "Error opening HIDBoot device: %s\n", usbErrorMessage(err));
        goto errorOccurred;
    }
    len = sizeof(buffer);
    if(endAddr > startAddr){    // we need to upload data
        if((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 1, buffer.bytes, &len)) != 0){
            fprintf(stderr, "Error reading page size: %s\n", usbErrorMessage(err));
            goto errorOccurred;
        }
        if(len < sizeof(buffer.info)){
            fprintf(stderr, "Not enough bytes in device info report (%d instead of %d)\n", len, (int)sizeof(buffer.info));
            err = -1;
            goto errorOccurred;
        }
        pageSize = getUsbInt(buffer.info.pageSize, 2);
        deviceSize = getUsbInt(buffer.info.flashSize, 4);
        printf("Page size   = %d (0x%x)\n", pageSize, pageSize);
        printf("Device size = %d (0x%x); %d bytes remaining\n", deviceSize, deviceSize, deviceSize - 2048);
        if(endAddr > deviceSize - 2048){
            fprintf(stderr, "Data (%d bytes) exceeds remaining flash size!\n", endAddr);
            err = -1;
            goto errorOccurred;
        }
        if(pageSize < 128){
            mask = 127;
        }else{
            mask = pageSize - 1;
        }
        startAddr &= ~mask;                  /* round down */
        endAddr = (endAddr + mask) & ~mask;  /* round up */
        printf("Uploading %d (0x%x) bytes starting at %d (0x%x)\n", endAddr - startAddr, endAddr - startAddr, startAddr, startAddr);
        while(startAddr < endAddr){
            buffer.data.reportId = 2;
            memcpy(buffer.data.data, dataBuffer + startAddr, 128);
            setUsbInt(buffer.data.address, startAddr, 3);
            printf("\r0x%05x ... 0x%05x", startAddr, startAddr + (int)sizeof(buffer.data.data));
            fflush(stdout);
            if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer.bytes, sizeof(buffer.data))) != 0){
                fprintf(stderr, "Error uploading data block: %s\n", usbErrorMessage(err));
                goto errorOccurred;
            }
            startAddr += sizeof(buffer.data.data);
        }
        printf("\n");
    }
    if(leaveBootLoader){
        /* and now leave boot loader: */
        buffer.info.reportId = 1;
        usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer.bytes, sizeof(buffer.info));
        /* Ignore errors here. If the device reboots before we poll the response,
         * this request fails.
         */
    }
errorOccurred:
    if(dev != NULL)
        usbCloseDevice(dev);
    return err;
}




typedef struct remoteDeviceInfo{
    uint8_t		reportId;
	uint8_t 	txStatus;
	uint8_t		payloadType;
    uint8_t   	pageSizeDiv2;
    uint8_t   	flashSizeInKB;
	uint8_t 	_padding[3];
}remoteDeviceInfo_t;

typedef struct progStatus{
	uint8_t		reportId;
	uint8_t 	txStatus;
	uint8_t		payloadType;
    uint8_t   	currentAddress[2];
	uint8_t 	_padding[3];
}progStatus_t;

typedef struct progCommand {
	uint8_t reportId;
	uint8_t cmd;
	uint8_t _padding[6];
} progCommand_t;

typedef struct remoteDeviceData{
    char    reportId;
    char    address[3];
    char    data[16];
} remoteDeviceData_t;


static int uploadDataRemote(char *dataBuffer, int startAddr, int endAddr)
{
usbDevice_t *dev = NULL;
int         err = 0, len, mask, pageSize, deviceSize, retry;
int 		currentAddr;
union{
    char            	bytes[1];
    remoteDeviceData_t  data;
	progCommand_t 		command;
}  txBuffer;

union {
	char 				bytes[1];
	remoteDeviceInfo_t	devinfo;
	progStatus_t 		status;
} replyBuffer;

	printf("\n\n");
    if((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM, IDENT_VENDOR_STRING, IDENT_PRODUCT_NUM, IDENT_PRODUCT_STRING, 1)) != 0) {
        fprintf(stderr, "Error opening '%s' device: %s\n", IDENT_PRODUCT_STRING, usbErrorMessage(err));
        goto errorOccurred;
    }
	printf("OPENED '%s' (VID:0x%04x PID:0x%04x) device\n", IDENT_PRODUCT_STRING, IDENT_VENDOR_NUM, IDENT_PRODUCT_NUM);
    if(endAddr > startAddr) {  // we need to upload data
		printf("\nUPLOAD: startAddress: %d(0x%x) endAddress: %d(0x%x)\n", startAddr, startAddr, endAddr, endAddr);
		printf("\nGETTING Remote device info ");
		retry = 5;
        while(retry) {
			putchar('.');
			txBuffer.command.reportId = 3;
			txBuffer.command.cmd = OTA_PROG_START;	/* Send START to remote device, which should reply with Device Info */
			if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, txBuffer.bytes, sizeof(txBuffer.command))) != 0) {
				//fprintf(stderr, "USBError: Getting device info: %s\n", usbErrorMessage(err));
				//goto errorOccurred;
				putchar('*');
			}
			len = sizeof(replyBuffer);	/* Get the reply from remote device */
			if((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 3, replyBuffer.bytes, &len)) != 0) {
				//fprintf(stderr, "USBError reading page size: %s\n", usbErrorMessage(err));
				//goto errorOccurred;
				putchar('*');
			}
			if(len < sizeof(replyBuffer.devinfo)) {
				fprintf(stderr, "Not enough bytes in device info report (%d instead of %d)\n", len, (int)sizeof(replyBuffer.devinfo));
				err = -1;
				goto errorOccurred;
			}
			if ((replyBuffer.devinfo.txStatus == 0) && (replyBuffer.devinfo.payloadType == PAYLD_TYPE_DEVINFO)) {	/* Valid reply received from remote device */
				break;
			}
			retry--;
			sleep_ms(10);
		}
		if(!retry) {
			fprintf(stderr, "\nCould not get Remote device info (Communication with Remote device failed!) txStatus:%d\n", replyBuffer.devinfo.txStatus);
			err = -1;
			goto errorOccurred;
		}
		/* We got the device info from remote. Parse page size and flash size from the data */
		pageSize = replyBuffer.devinfo.pageSizeDiv2 * 2;
        deviceSize = replyBuffer.devinfo.flashSizeInKB * 1024;
		//printf("Retry: %d\t", retry);
        printf("\nPage size   = %d (0x%x)\t", pageSize, pageSize);
        printf("Device size = %d (0x%x); %d bytes remaining\n", deviceSize, deviceSize, deviceSize - 2048);
        if(endAddr > deviceSize - 2048){
            fprintf(stderr, "Data (%d bytes) exceeds remaining flash size!\n", endAddr);
            err = -1;
            goto errorOccurred;
        }
        if(pageSize < 128){
            mask = 127;
        }else{
            mask = pageSize - 1;
        }
        startAddr &= ~mask;                  /* round down */
        endAddr = (endAddr + mask) & ~mask;  /* round up */
		/* Transmit the data blocks now */
		printf("UPLOADING %d (0x%x) bytes starting at %d (0x%x) END: %d(0x%x)\n", endAddr - startAddr, endAddr - startAddr, startAddr, startAddr, endAddr, endAddr);
        sleep_ms(10);
		while(startAddr < endAddr) {
            txBuffer.data.reportId = 4;
            memcpy(txBuffer.data.data, dataBuffer + startAddr, sizeof(txBuffer.data.data));
            setUsbInt(txBuffer.data.address, startAddr, 3);
            printf("\n0x%05x ... 0x%05x: ", startAddr, startAddr + (int)sizeof(txBuffer.data.data));
            fflush(stdout);
			retry = 5;
			while(retry) {
				putchar('.');
				/* Send data block to remote device */
				if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, txBuffer.bytes, sizeof(txBuffer.data))) != 0) {
					//fprintf(stderr, "USBError uploading data block: %s\n", usbErrorMessage(err));
					//goto errorOccurred;
					putchar('*');
				}
				//sleep_ms(10);
				len = sizeof(replyBuffer); /* Get the reply from remote device */
				if((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 3, replyBuffer.bytes, &len)) != 0) {
					//fprintf(stderr, "USBError getting status: %s\n", usbErrorMessage(err));
					//goto errorOccurred;
					putchar('*');
				}
				if(len < sizeof(replyBuffer.status)) {
					fprintf(stderr, "Not enough bytes in device info report (%d instead of %d)\n", len, (int)sizeof(replyBuffer.status));
					err = -1;
					goto errorOccurred;
				}
				if ((replyBuffer.status.txStatus == 0) && (replyBuffer.status.payloadType == PAYLD_TYPE_STATUS)) {
					currentAddr = getUsbInt((char *)&replyBuffer.status.currentAddress[0], 2);
					if(currentAddr == startAddr + (int)sizeof(txBuffer.data.data)) {
						printf("OK");
						break;
					}
				}
				//printf("\nTxStatus: %d.. Addr: %d\n", replyBuffer.info.txStatus, getUsbInt(replyBuffer.info.pageSize, 2));
				sleep_ms(10);
				retry--;
			}
			if(!retry) {
				fprintf(stderr, "ERROR: programming failed at address 0x%05x\n", startAddr);
				printf("txStatus: %d, Type: 0x%02x, Addr: 0x%x\n", replyBuffer.status.txStatus, replyBuffer.status.payloadType, getUsbInt((char *)&replyBuffer.status.currentAddress[0], 2));
				goto errorOccurred;
			}
			//printf("txStatus: %d, Type: 0x%02x, Addr: 0x%x\n", replyBuffer.info.txStatus, replyBuffer.info.payloadType, getUsbInt((char *)&replyBuffer.info.flashSizeInKB, 2));
            startAddr += sizeof(txBuffer.data.data);
			//sleep_ms(10);
        }
		printf("\n\nENDING communication ");
		sleep_ms(10);
		txBuffer.command.reportId = 3;
		txBuffer.command.cmd = OTA_PROG_STOP;	/* Send STOP to remote device */
		retry = 5;
		while(retry) {
			putchar('.');
			if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, txBuffer.bytes, sizeof(txBuffer.command))) != 0) {
				//fprintf(stderr, "USBError: Sending STOP command: %s\n", usbErrorMessage(err));
				//goto errorOccurred;
				putchar('*');
			}
			len = sizeof(replyBuffer);	/* Get the reply from remote device */
			if((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 3, replyBuffer.bytes, &len)) != 0) {
				//fprintf(stderr, "USBError: Getting STOP response: %s\n", usbErrorMessage(err));
				//goto errorOccurred;
				putchar('*');
			}
			if ((replyBuffer.status.txStatus == 0) && (replyBuffer.status.payloadType == PAYLD_TYPE_STATUS)) {	/* Valid reply received from remote device */
				if(replyBuffer.status.currentAddress[0] == OTA_PROG_FINISHED) {
					printf("OK");
					break;
				}
			}
			retry--;
			sleep_ms(10);
		}
		if(!retry) {
			fprintf(stderr, "\nERROR: ending communication failed (Retry: %d)\n", retry);
			goto errorOccurred;
		}
		/* Reset the remote device */
		printf("\nRESETTING remote device ");
		///sleep_ms(200);
		txBuffer.command.reportId = 3;
		txBuffer.command.cmd = OTA_PROG_BOOT;	/* Send BOOT to remote device */
		retry = 5;
		while(retry) {
			putchar('.');
			if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, txBuffer.bytes, sizeof(txBuffer.command))) != 0) {
				//fprintf(stderr, "USBError: Sending BOOT command: %s\n", usbErrorMessage(err));
				//goto errorOccurred;
				putchar('*');
			}
			len = sizeof(replyBuffer);	/* Get the reply from remote device */
			if((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 3, replyBuffer.bytes, &len)) != 0) {
				fprintf(stderr, "USBError: Getting BOOT response: %s\n", usbErrorMessage(err));
				goto errorOccurred;
			}
			if(replyBuffer.status.txStatus == 0) {	/* Valid reply received from remote device */
				printf("OK");
				break;
			}
			retry--;
			sleep_ms(10);
		}
		printf("\n");
    }
	if(leaveBootLoader) {
        /* and now leave boot loader: */
        txBuffer.command.reportId = 1;
        usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, txBuffer.bytes, sizeof(txBuffer.command));
        /* Ignore errors here. If the device reboots before we poll the response,
         * this request fails.
         */
    }
	
errorOccurred:
    if(dev != NULL)
        usbCloseDevice(dev);
    return err;
}




/* ------------------------------------------------------------------------- */

static void printUsage(char *pname)
{
    fprintf(stderr, "usage: %s [remote] [-r] [<intel-hexfile>]\n", pname);
}

int main(int argc, char **argv)
{
char    *file = NULL;
bool	remoteBoot = false;
int 	count = 1;

    if(argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printUsage(argv[0]);
        return 1;
    }
	argv++;
	if(strcmp(*argv, "remote") == 0) {
		remoteBoot = true;
		argv++;
		count++;
	}
	if((count < argc) && (strcmp(*argv, "-r") == 0)) {
		leaveBootLoader = 1;
		argv++;
		count++;
	}
	if(count < argc) {
		file = *argv;
	}
#if 0
	if(strcmp(argv[1], "remote") == 0) {
		if(strcmp(argv[2], "-r") == 0){
			leaveBootLoader = 1;
			if(argc >= 4){
				file = argv[3];
			}
		}else{
			file = argv[2];
		}
		remoteBoot = true;
	}
	else {
		if(strcmp(argv[1], "-r") == 0){
			leaveBootLoader = 1;
			if(argc >= 3){
				file = argv[2];
			}
		}else{
			file = argv[1];
		}
	}
#endif

    startAddress = sizeof(dataBuffer);
    endAddress = 0;
    if(file != NULL){   // an upload file was given, load the data
        memset(dataBuffer, -1, sizeof(dataBuffer));
        if(parseIntelHex(file, dataBuffer, &startAddress, &endAddress))
            return 1;
        if(startAddress >= endAddress){
            fprintf(stderr, "No data in input file, exiting.\n");
            return 0;
        }
    }
    // if no file was given, endAddress is less than startAddress and no data is uploaded
	if(remoteBoot) {
		if(uploadDataRemote(dataBuffer, startAddress, endAddress))
			return 1;		
	}
	else {
		if(uploadData(dataBuffer, startAddress, endAddress))
			return 1;
	}
    return 0;
}

/* ------------------------------------------------------------------------- */


