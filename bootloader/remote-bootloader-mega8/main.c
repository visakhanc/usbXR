/*	
 *	main.c
 *	Author: Visakhan C
 *	Created: Nov 16, 2017
 *	Project: Remote bootloader
 */

#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/boot.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include "rfm70.h"
#include "../bootloader_defs.h"

//#define ACCELEROMETER_BOARD
//#define OLED_BOARD
#define PROTO_BOARD

//#define TEST_MODE 

/* Board specific definitions */
#ifdef ACCELEROMETER_BOARD
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
#endif

#ifdef OLED_BOARD
	#define RED_LED					PB0
	#define RED_LED_DDR				DDRB
	#define RED_LED_PORT			PORTB
	#define RED_LED_INIT()			(RED_LED_DDR |= (1 << RED_LED))
	#define RED_LED_ON()			(RED_LED_PORT |= (1 << RED_LED))
	#define RED_LED_OFF()			(RED_LED_PORT &= ~(1 << RED_LED))
	#define RED_LED_TOGGLE()		(RED_LED_PORT ^= (1 << RED_LED))

	#define BUTTON					PD6
	#define BUTTON_PORT				PORTD
	#define BUTTON_PIN				PIND
	#define BUTTON_INIT()			(BUTTON_PORT |= (1 << BUTTON))
	#define BUTTON_PRESSED()		((BUTTON_PIN & (1 << BUTTON)) == 0)
#endif

#ifdef PROTO_BOARD
	#define RED_LED					PB0
	#define RED_LED_DDR				DDRB
	#define RED_LED_PORT			PORTB
	#define RED_LED_INIT()			(RED_LED_DDR |= (1 << RED_LED))
	#define RED_LED_ON()			(RED_LED_PORT |= (1 << RED_LED))
	#define RED_LED_OFF()			(RED_LED_PORT &= ~(1 << RED_LED))
	#define RED_LED_TOGGLE()		(RED_LED_PORT ^= (1 << RED_LED))

	#define BUTTON					PD7
	#define BUTTON_PORT				PORTD
	#define BUTTON_PIN				PIND
	#define BUTTON_INIT()			(BUTTON_PORT |= (1 << BUTTON))
	#define BUTTON_PRESSED()		((BUTTON_PIN & (1 << BUTTON)) == 0)
#endif


/* Application validity status stored in EEPROM */
#define APP_VALIDITY_LOCATION	E2END	/* Location to store application validity flag: Last location of EEPROM */
#define VALID_APP				0xaa	/* Indicates: a valid program is present */
#define INVALID_APP				0xff	/* Indicates: no valid program present (Reset state of an EEPROM byte) */
#define GET_APP_VALIDITY()		(eeprom_read_byte((const uint8_t *)APP_VALIDITY_LOCATION))
#define SET_APP_VALIDITY(x)		(eeprom_write_byte((uint8_t *)APP_VALIDITY_LOCATION, (uint8_t)(x)))

#if (FLASHEND) > 0xffff /* we need long addressing */
#   define addr_t           	uint32_t
#else
#   define addr_t           	uint16_t
#endif


void Receive_Packet(void);

volatile static uint8_t sec_count;  /* Seconds elapsed */
static bool can_exit = false;	/* Can we exit bootloader if a timeout happens? */
static uint8_t rx_buf[CONFIG_RFM70_STATIC_PL_LENGTH];
static uint8_t addr[CONFIG_RFM70_ADDR_LEN] = CONFIG_RFM70_ADDRESS;
static uint8_t flash_info[] =  {	
	PAYLD_TYPE_DEVINFO,
	(uint8_t)(SPM_PAGESIZE >> 1), 		/* PageSize in multiple of 2 */
    (uint8_t)((FLASHEND + 1) >> 10),	/*  Flash size in multiple of 1kB */
};
static uint8_t ack_pld[3];
static void (*nullVector)(void) __attribute__((__noreturn__));

static void leaveBootloader(void)
{
    cli();
    boot_rww_enable();
	TCCR0 = 0;
    GICR = (1 << IVCE);     /* enable change of interrupt vectors */
    GICR = (0 << IVSEL);    /* move interrupts to application flash section */
/* We must go through a global function pointer variable instead of writing
 *  ((void (*)(void))0)();
 * because the compiler optimizes a constant 0 to "rcall 0" which is not
 * handled correctly by the assembler.
 */
    nullVector();
}


int main(void)
{
	uint8_t i;
	
	BUTTON_INIT();
	RED_LED_INIT();
	RED_LED_OFF();
	_delay_ms(10); 
	if(GET_APP_VALIDITY() == VALID_APP) {
		can_exit = true;
	}
	if(BUTTON_PRESSED()) {
		RED_LED_ON();
		/* Delay to wait for the bootloader button to release */
		_delay_ms(1000); 
		RED_LED_OFF();
	}
	else {
		if(can_exit) {
			leaveBootloader();
		}
	}
	GICR = (1 << IVCE);     /* enable change of interrupt vectors */
    GICR = (1 << IVSEL);    /* move interrupts to boot flash section */	
	if(0 != rfm70_init(RFM70_MODE_PRX, addr)) { /* Initialize RFM70 module in Rx mode */
		RED_LED_ON();
	}
	/* Setup Timer0 for calculating timeout */ 
	TCCR0 = 0x5; 	/* Timer clock = 8MHz / 1024 = 128us  */
	TIMSK = (1 << TOIE0); 
	sei();   // enable interrupts globally
	/* Initially, populate the ACK payload with Flash info */
	i = sizeof(flash_info) - 1;
	do {
		ack_pld[i] = flash_info[i];
	} while(i--);
	rfm70_set_ack_payload(RFM70_PIPE0, ack_pld, sizeof(ack_pld));
	
	while(1)
	{
		/* Receive and process packets */
		Receive_Packet();
		/* Jump to application on 60 sec inactivity */
		if((sec_count > 60) && (can_exit)) {
			leaveBootloader();
		}
	}
}



void Receive_Packet(void)
{
	static addr_t  currentAddress = 0; /* in bytes */
	uint8_t i, len; 
	uint8_t *data;
	union {
		addr_t  l;
		uint16_t  s[sizeof(addr_t)/2];
		uint8_t   c[sizeof(addr_t)];
	} address;

	rfm70_receive_packet(rx_buf, &len);
	if(len == 0) { /* No packet received */
		return;
	}
	else if(len == 4) {
		if(OTA_PROG_BOOT == rx_buf[0]) { /* Final packet of the protocol */ 
			/* Set Valid flag in EEPROM */
			if(GET_APP_VALIDITY() != VALID_APP) {
				SET_APP_VALIDITY(VALID_APP);
				eeprom_busy_wait();
			}
			/* Boot into the application */
			leaveBootloader();
		}
		else if(OTA_PROG_STOP == rx_buf[0]) { /* Indicates that all data blocks have been sent */
			/* Change ACK payload to indicate SUCCESS */
			ack_pld[0] = PAYLD_TYPE_STATUS;
			ack_pld[1] = OTA_PROG_FINISHED;
		}
		else if(OTA_PROG_START == rx_buf[0]) { /* This is the initial packet from Host, requesting for flash info */
			/* We send auto-ack payload containing device info */
			i = sizeof(flash_info) - 1;
			do {
				ack_pld[i] = flash_info[i];
			} while(i--);
			currentAddress = 0;
			//curBlock = 0;
		}
	}
	else if(len == 19) { /* data block */
		data = rx_buf;
		address.c[0] = data[0];
		address.c[1] = data[1];
#if (FLASHEND) > 0xffff /* we need long addressing */
		address.c[2] = data[2];
		address.c[3] = 0;
#endif
		if(currentAddress == address.l) {
			//RED_LED_TOGGLE();
			/* Make the application is invalid, when we start programming */
			if(0 == currentAddress) {
				if(GET_APP_VALIDITY() != INVALID_APP) {
					SET_APP_VALIDITY(INVALID_APP);
					eeprom_busy_wait();
				}
				can_exit = false;
			}
			data += 3;
			len -= 3;
			do {
				addr_t prevAddr;
		#if SPM_PAGESIZE > 256
				uint16_t pageAddr;
		#else
				uint8_t pageAddr;
		#endif
				pageAddr = address.s[0] & (SPM_PAGESIZE - 1);
				
				if(pageAddr == 0){              /* if page start: erase */
		#ifndef TEST_MODE
					cli();
					boot_page_erase(address.l); /* erase page */
					sei();
					boot_spm_busy_wait();       /* wait until page is erased */
		#endif
				}
		#ifndef TEST_MODE
				cli();
				boot_page_fill(address.l, *(uint16_t *)data);
				sei();
		#endif
				prevAddr = address.l;
				address.l += 2;
				data += 2;
				/* write page when we cross page boundary */
				pageAddr = address.s[0] & (SPM_PAGESIZE - 1);
				if(pageAddr == 0){
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
			/* set ack payload */
			i = 0;
			ack_pld[i++] = PAYLD_TYPE_STATUS;
			ack_pld[i++] = currentAddress & 0xff;
			ack_pld[i] = currentAddress >> 8;
		}
	}
	rfm70_set_ack_payload(RFM70_PIPE0, ack_pld, sizeof(ack_pld));
}



ISR(TIMER0_OVF_vect)
{
	static uint8_t ovf = 0;
	
	if(++ovf > 30) {
		ovf = 0;
		sec_count++;
		if(!can_exit) {
			RED_LED_ON();
		}
	}
	if(1 == ovf) { 
		RED_LED_OFF();	/* 32ms blink each second while programming */
	}
}