/* Name: main.c
 * Project: 
 * Author: 
 * Creation Date: 
 * Tabsize: 
 */

#include <stdbool.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */
#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "rfm70.h"

/*************** DEFINES & MACROS *******************/

#define RED_LED					PC1
#define RED_LED_DDR				DDRC
#define RED_LED_PORT			PORTC
#define RED_LED_INIT()			(RED_LED_DDR |= (1 << RED_LED))
#define RED_LED_OFF()			(RED_LED_PORT |= (1 << RED_LED))
#define RED_LED_ON()			(RED_LED_PORT &= ~(1 << RED_LED))
#define RED_LED_TOGGLE()		(RED_LED_PORT ^= (1 << RED_LED))

#define BUTTON					PC0
#define BUTTON_PORT				PORTC
#define BUTTON_PIN				PINC
#define BUTTON_INIT()			(BUTTON_PORT |= (1 << BUTTON))
#define BUTTON_PRESSED()		((BUTTON_PIN & (1 << BUTTON)) == 0)


/* HID Input report structure */
typedef struct {
	uint8_t report_id;
	uint8_t data[7];
} input_report_t;



/******************** VARIABLES **********************/

static uchar	replyBuffer[8]; /* Buffer for HID Get Feature Report request */
static input_report_t  input_report_buf = {.report_id = 2};
static uint8_t addr[CONFIG_RFM70_ADDR_LEN] = CONFIG_RFM70_ADDRESS;
//static uint8_t tx_buf[CONFIG_RFM70_STATIC_PL_LENGTH];
static uint8_t rx_buf[CONFIG_RFM70_STATIC_PL_LENGTH];


/************** FUNCTION DECLARATIONS ***************/

static void AVR_Init(void);



/* ----------------------------- USB interface ----------------------------- */
PROGMEM const char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {    /* USB report descriptor */

    0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor Defined)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)

	0x85, 0x01,                    //   REPORT_ID (1)
    0x95, 0x07,                    //   REPORT_COUNT (7)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

	0x85, 0x02,					   // 	REPORT_ID (2)
	0x95, 0x07,                    //   REPORT_COUNT (7)
    0x09, 0x00,                    //   USAGE (Undefined)
    0x81, 0x02,              		//  INPUT (Data,Var,Abs)
		
    0xc0                           // END_COLLECTION
};

 

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT) {  
			/* wValue: ReportType (highbyte), ReportID (lowbyte) */
			if(rq->wValue.bytes[1] == 0x01) {	/* Get Input Report */
				usbMsgPtr = (usbMsgPtr_t)&input_report_buf;
				return sizeof(input_report_buf);
			}
			else {
				replyBuffer[0] = 1;
				usbMsgPtr = (usbMsgPtr_t)replyBuffer;
				return sizeof(replyBuffer);
			}
        }
		else if(rq->bRequest == USBRQ_HID_SET_REPORT) {		/* Set Feature/Output report */
			return USB_NO_MSG; /* pass on to usbFunctionWrite() */
		}
    } else {
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
	if(1 == data[0]) {	/* Report ID:1 -> Send Command */
	
	}
	return 1;
}

/* ------------------------------------------------------------------------- */




int main(void)
{
	uchar   i;
	uint8_t  len;
	bool send = false;
	
    wdt_disable();
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
    AVR_Init();
	usbInit();
	rfm70_init(RFM70_MODE_PRX, addr);
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i) {             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    sei();

    for(;;) {                /* main event loop */
        wdt_reset();
        usbPoll();
		rfm70_receive_packet(rx_buf, &len);
		if(len) {
			RED_LED_TOGGLE();
			for(i = 0; i < sizeof(input_report_buf.data); i++) {
				input_report_buf.data[i] = rx_buf[i];
			}
			send = true;
		}
		if(usbInterruptIsReady() && (send == true)) {
			usbSetInterrupt((uint8_t *)&input_report_buf, sizeof(input_report_buf));
			send = false;
		}
		/* If button is pressed cause a reset by watchdog. If pressed for longer time (>500ms), bootloader can be entered */
		if(BUTTON_PRESSED()) {
			wdt_enable(WDTO_500MS);
			_delay_ms(1000); /* Watchdog times out during this */
		}
    }
}



static void AVR_Init(void)
{
	RED_LED_INIT();
	RED_LED_ON();
	_delay_ms(200);
	RED_LED_OFF();
	BUTTON_INIT();
}




/* ------------------------------------------------------------------------- */
