/*
 *  board.h
 *
 *	Pin definitions and Macros for usbXR
 *
 *  Created on: Jan 29, 2026
 *      Author: Visakhan
 */

#ifndef BOARD_H_
#define BOARD_H_


#include <avr/io.h>


/* Button and LED */
#define LED						PD6
#define LED_DDR					DDRD
#define LED_PORT				PORTD

#define BUTTON					PD5
#define BUTTON_PORT				PORTD
#define BUTTON_PIN				PIND

/* Macros for Button and LED */
#define LED_INIT()				(LED_DDR |= (1 << LED))
#define LED_OFF()				(LED_PORT |= (1 << LED))
#define LED_ON()				(LED_PORT &= ~(1 << LED))
#define LED_TOGGLE()			(LED_PORT ^= (1 << LED))
#define BUTTON_INIT()			(BUTTON_PORT |= (1 << BUTTON))
#define BUTTON_PRESSED()		((BUTTON_PIN & (1 << BUTTON)) == 0)


#endif /* BOARD_H_ */
