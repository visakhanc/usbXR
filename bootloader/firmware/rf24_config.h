/*
 * 	rf24_config.h
 *
 *	This file allows for modification of various parameters of the RF module for the project
 */

#ifndef RF24_CONFIG_H_
#define RF24_CONFIG_H_



/******** I/O PIN DEFINITIONS FOR AVR *********/

/* Define which AVR pin is connected to the RF module Chip Enable (CE) */
#define CE_DDR		DDRB
#define CE_PORT	PORTB
#define CE_PIN   	1

/****************** POLLED/INTERRUPT MODE *******************/
/* Define to 0 if interrupt mode is used. In this case AVR INT1
   pin should be connected to RF Module's IRQ pin
   Define to 1 if polled mode is used. In this case RFM70 IRQ pin
   is not connected to AVR
*/
#define CONFIG_RF24_POLLED_MODE 			                  0


/************ FOR RFM7x Modules ONLY ************/
/* Define to 1 if RFM70/RFM73/RFM75 module is used. This will add
 * necessary initialization for extra Bank1 registers of the RFM7x module */
#define RFM7x_INIT	0



 /* Address of the radio and Address length
  * example:
  *	#define RF24_ADDRESS		{0x11, 0x22, 0x33, 0x44, 0x55}
  *	#define RF24_ADDR_LEN		5
  *Note: Length can be 3, 4, 5
  *	  If Automatic ack is enabled and if mode is PTX, this address is used to set RX_P0 address
  */
#define CONFIG_RF24_ADDRESS					               	{0xFC, 0xFC, 0xFC, 0xFC, 0xFC}
#define CONFIG_RF24_ADDR_LEN				               	5


/* Enable or Disable Automatic Retransmit/Acknowledgement feature
 * To enable define to 1, otherwise to 0
 * NOTE: In PTX/PRX, EN_AA=0 for No ack; In PRX/PTX, NO_ACK is checked in the packet
 */
 #define CONFIG_RF24_AUTOACK_ENABLED 		               	1


/* Whether to enable or disable dynamic payload width
 * Define as 1 to enable, 0 to disable
 */
#define CONFIG_RF24_DYNAMIC_PL_ENABLED		               	1

/* Length of the static payload
 * Define this from 0 to 32 (bytes)
 */
#define CONFIG_RF24_STATIC_PL_LENGTH		               	32


/* Whether to enable Payload in the ACK
   If defined to 1, also define ACK payload length */
#define CONFIG_RF24_ACK_PL_ENABLED			               	1
#define CONFIG_RF24_ACK_PL_LENGTH			               	6


/* Output power
 * Define this to:
 * 	RF24_PWR_0DBM : 0dBm
 * 	RF24_PWR_M6DBM : -6dBm
 * 	RF24_PWR_M12DBM : -12dBm
 * 	RF24_PWR_M18DBM : -18dBm
 */
#define CONFIG_RF24_TX_PWR					               	RF24_PWR_0DBM


/* Data rate
 *	Define this to:
 *		RF24_RATE_250KBPS
 *		RF24_RATE_1MBPS
 *		RF24_RATE_2MBPS
 */
#define CONFIG_RF24_DATA_RATE								RF24_RATE_2MBPS


/* Define RF channel */
#define CONFIG_RF24_RF_CHANNEL 				       			100


/* Define how many retransmitts that should be performed */
#define CONFIG_RF24_TX_RETRANSMITS			               	15



#endif /* RF24_CONFIG_H_ */
