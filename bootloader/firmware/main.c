/* Name: main.c
 * Project: AVR bootloader HID
 * Author: Christian Starkjohann
 * Creation Date: 2007-03-19
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt)
 * This Revision: $Id$
 *
 * Modified: Nov-2017
 * By: Visakhan C
 * For: usbXR project: https://github.com/visakhanc/usbXR
 * Modifications: Added functionality for over-the-air programming of remote devices using NRF24L01 RF interface
 */

#include <stdbool.h>
#include <string.h>  /* memcpy() */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/boot.h>
#include "rf24.h"
#include "rf24_config.h"
#include "usbdrv.h"
#include "bootloader_defs.h"




/******************** MACROS AND DEFINITIONS **********************/

#if (FLASHEND) > 0xffff /* we need long addressing */
#   define addr_t           uint32_t
#else
#   define addr_t           uint16_t
#endif

/* allow compatibility with avrusbboot's bootloaderconfig.h: */
#ifdef BOOTLOADER_INIT
#   define bootLoaderInit()         BOOTLOADER_INIT
#endif
#ifdef BOOTLOADER_CONDITION
#   define bootLoaderCondition()    BOOTLOADER_CONDITION
#endif

/* compatibility with ATMega88 and other new devices: */
#ifndef TCCR0
#define TCCR0   TCCR0B
#endif
#ifndef GICR
#define GICR    MCUCR
#endif


/* HID Input report structure */
typedef struct {
	uint8_t reportId;
	uint8_t data[7];
} hidReport_t;




/********************** GLOBAL VARIABLES **************************/

#if BOOTLOADER_CAN_EXIT
static uint8_t	exitMainloop;
#endif
static addr_t   currentAddress; /* in bytes */
static uint8_t	offset;         /* data already processed in current transfer */
static uint8_t  replyBuffer[7] = {
        1,     /* report ID */
        SPM_PAGESIZE & 0xff,
        SPM_PAGESIZE >> 8,
        ((long)FLASHEND + 1) & 0xff,
        (((long)FLASHEND + 1) >> 8) & 0xff,
        (((long)FLASHEND + 1) >> 16) & 0xff,
        (((long)FLASHEND + 1) >> 24) & 0xff
    };

#if defined(__AVR_ATmega328P__)
static bool		remoteBoot;
static hidReport_t	replyBufferRemote = {.reportId = 3};
static uint8_t 	txBuf[CONFIG_RF24_STATIC_PL_LENGTH];
static uint8_t 	rxBuf[CONFIG_RF24_STATIC_PL_LENGTH];
static uint8_t 	addr[CONFIG_RF24_ADDR_LEN] = CONFIG_RF24_ADDRESS;
static uint8_t	recv_len;
static bool bootInProgress = false;
static volatile bool bootAckPld = false;  /* Transmit ACK payload for boot request */
static uint8_t ackPld[2];
#endif

	
	
const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor defined)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)

	0x85, 0x01,                    //   REPORT_ID (1)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, 0x02,                    //   REPORT_ID (2)
    0x95, 0x83,                    //   REPORT_COUNT (131)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

#if defined(__AVR_ATmega328P__)
    0x85, 0x03,                    //   REPORT_ID (3)
    0x95, 0x07,                    //   REPORT_COUNT (7)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, 0x04,                    //   REPORT_ID (4)
    0x95, 0x13,                    //   REPORT_COUNT (19)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
#endif

    0xc0                           // END_COLLECTION
};





/******************** FUNCTION DECLARATIONS  **********************/

/* As the application might use the Watchdog Timer for a software reset, this function is needed to disable WDT as early as possible during initialization */
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));
void get_mcusr(void) \
  __attribute__((naked)) \
  __attribute__((section(".init3")));

void get_mcusr(void)
{
  mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
}



static void (*nullVector)(void) __attribute__((__noreturn__));
static void leaveBootloader(void)
{
    cli();
    boot_rww_enable();
    USB_INTR_ENABLE = 0;
    USB_INTR_CFG = 0;       /* also reset config bits */
#if F_CPU == 12800000
    TCCR0 = 0;              /* default value */
#endif
    GICR = (1 << IVCE);     /* enable change of interrupt vectors */
    GICR = (0 << IVSEL);    /* move interrupts to application flash section */
/* We must go through a global function pointer variable instead of writing
 *  ((void (*)(void))0)();
 * because the compiler optimizes a constant 0 to "rcall 0" which is not
 * handled correctly by the assembler.
 */
    nullVector();
}



uint8_t   usbFunctionSetup(uint8_t data[8])
{
usbRequest_t    *rq = (void *)data;

    if(USBRQ_HID_SET_REPORT == rq->bRequest) {
	    if(rq->wValue.bytes[0] > 1) {
#if defined(__AVR_ATmega328P__)
			remoteBoot = (rq->wValue.bytes[0] == 2) ? false : true;
#endif
            offset = 0;
            return USB_NO_MSG;  /* Process the packet in usbFunctionWrite() */
        }
#if BOOTLOADER_CAN_EXIT
        else if(rq->wValue.bytes[0] == 1) {
            exitMainloop = 1;
        }
#endif
    }
	else if(rq->bRequest == USBRQ_HID_GET_REPORT) {
		if(rq->wValue.bytes[0] == 1) {
			usbMsgPtr = (usbMsgPtr_t)replyBuffer;
			return sizeof(replyBuffer);
		}
#if defined(__AVR_ATmega328P__)
		else if(rq->wValue.bytes[0] == 3) {
			usbMsgPtr = (usbMsgPtr_t)&replyBufferRemote;
			return sizeof(replyBufferRemote);
		}
#endif
    }
    return 0;
}



uint8_t usbFunctionWrite(uint8_t *data, uint8_t len)
{
union {
    addr_t  l;
    uint16_t    s[sizeof(addr_t)/2];
    uint8_t   c[sizeof(addr_t)];
}   address;
	uint8_t	isLast = 0;

#if defined(__AVR_ATmega328P__)
	if(remoteBoot) {
		replyBufferRemote.data[1] = 0;	/* Clear byte to validate received data */
		if(offset == 0) {  /* Report ID */
			if(data[0] == 3) {	/* Report ID:3 -> Command */
				if(data[2] == CMD_OTA_BOOT_START) {
					ackPld[0] = data[1]; /* Device ID */
					ackPld[1] = data[2]; /* Command to be sent */
					rf24_flush_txfifo(); /* Flush pending ack payloads if any */
					rf24_set_ack_payload(RF24_PIPE0, ackPld, sizeof(ackPld));
					bootAckPld = true;
				}
				else if(data[2] == CMD_OTA_BOOT_END) {
					bootInProgress = false;
					bootAckPld = false;
					rf24_rx_mode();
				}
				else if(data[2] == CMD_OTA_BOOT_TXMODE) {
					bootInProgress = true;
					rf24_tx_mode();
				}
				else {  /* transmit other commands to remote */
					txBuf[0] = data[1];
					txBuf[1] = data[2];
					if(0 == (replyBufferRemote.data[0] = rf24_transmit_packet(txBuf, 2))) {
						rf24_receive_packet(&replyBufferRemote.data[1], &recv_len);
					}
				}
				return 1;
			}
			data++;  /* Skip report ID */
			len--;
		}
		while(len--) {
			txBuf[offset++] = *data++;
		}
		if(19 == offset) {  /* whole block received, now send the packet to remote */
			isLast = 1;
			if(0 == (replyBufferRemote.data[0] = rf24_transmit_packet(txBuf, offset))) {
				LED_TOGGLE();
				rf24_receive_packet(&replyBufferRemote.data[1], &recv_len);
			}
		}
		return isLast;
	}
#endif

	address.l = currentAddress;
	if(0 == offset) {
		address.c[0] = data[1];
		address.c[1] = data[2];
#if (FLASHEND) > 0xffff /* we need long addressing */
		address.c[2] = data[3];
		address.c[3] = 0;
#endif
		data += 4;
		len -= 4;
	}
	offset += len;
	isLast = offset & 0x80; /* != 0 if last block received */
	do {
		addr_t prevAddr;
#if SPM_PAGESIZE > 256
		uint16_t pageAddr;
#else
		uint8_t pageAddr;
#endif
		pageAddr = address.s[0] & (SPM_PAGESIZE - 1);
		if(0 == pageAddr) {              /* if page start: erase */
#ifndef TEST_MODE
			cli();
			boot_page_erase(address.l); /* erase page */
			sei();
			boot_spm_busy_wait();       /* wait until page is erased */
#endif
		}
		cli();
		boot_page_fill(address.l, *(short *)data);
		sei();
		prevAddr = address.l;
		address.l += 2;
		data += 2;
		/* write page when we cross page boundary */
		pageAddr = address.s[0] & (SPM_PAGESIZE - 1);
		if(0 == pageAddr){
#ifndef TEST_MODE
			cli();
			boot_page_write(prevAddr);
			sei();
			boot_spm_busy_wait();
#endif
		}
		len -= 2;
	} while(len);
	currentAddress = address.l;

	return isLast;
}


static void initForUsbConnectivity(void)
{
uint8_t   i = 0;

#if F_CPU == 12800000
    TCCR0 = 3;          /* 1/64 prescaler */
#endif
    usbInit();
    /* enforce USB re-enumerate: */
    usbDeviceDisconnect();  /* do this while interrupts are disabled */
    do{             /* fake USB disconnect for > 250 ms */
        _delay_ms(1);
    } while(--i);
    usbDeviceConnect();
    sei();
}


int main(void)
{
#if defined(__AVR_ATmega328P__)
	uint8_t  len;
#endif

    /* initialize hardware */
    bootLoaderInit();	
	if(bootLoaderCondition()) {
		uchar i = 0, j = 0;

#ifndef TEST_MODE
        GICR = (1 << IVCE);  /* enable change of interrupt vectors */
        GICR = (1 << IVSEL); /* move interrupts to boot flash section */
#endif

        initForUsbConnectivity();
#if defined(__AVR_ATmega328P__)
		if(0 != rf24_init(RF24_MODE_PRX, addr)) {
			LED_ON();
		}
#endif
        do {  /* main event loop */
            usbPoll();
#if defined(__AVR_ATmega328P__)
    		if(!bootInProgress) {
    			rf24_receive_packet(rxBuf, &len);
    			if(len) {
    				LED_TOGGLE();
    				if(rxBuf[1] == STATUS_TYPE_DEVINFO) {
    					if((rxBuf[2] == STATUS_OTA_BOOT_REQ) || (rxBuf[2] == STATUS_OTA_BOOT_READY)) {
    						cli();
    						memcpy(replyBufferRemote.data, rxBuf, len);
    						sei();
    						if(bootAckPld) {
    							rf24_set_ack_payload(RF24_PIPE0, ackPld, sizeof(ackPld));
    						}
    					}
    				}
    			}
    		}
#endif

#if BOOTLOADER_CAN_EXIT
            if(exitMainloop) {
#if F_CPU == 12800000
                break;  /* memory is tight at 12.8 MHz, save exit delay below */
#endif
                if(--i == 0) {
                    if(--j == 0)
                        break;
                }
            }
#endif
        } while(1);
	}
	leaveBootloader();
}

