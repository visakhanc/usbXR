/* 
 *  rfm70_config.h
 *
 *	This file defines the configurations to be used for RFM70 module 
 *	Modify these definitions according to your project
 */

#ifndef RFM70_CONFIG_H
#define RFM70_CONFIG_H


/******** I/O PIN DEFINITIONS FOR AVR *********/

/* Define which AVR pin is connected to RFM70 Chip Enable (CE) */
#define CE_DDR		DDRB
#define CE_PORT		PORTB
#define CE_PIN   	1

/****************** POLLED/INTERRUPT MODE *******************/
/* Define to 0 if interrupt mode is used. In this case AVR INT1 
   pin should be connected to RFM70 IRQ pin
   Define to 1 if polled mode is used. In this case RFM70 IRQ pin
   is not connected to AVR
*/
#define CONFIG_RFM70_POLLED_MODE 			0


/**********************************************/
 /* Address of the radio and Address length 
  * example: 
  *	#define RFM70_ADDRESS		{0x11, 0x22, 0x33, 0x44, 0x55}
  *	#define RFM70_ADDR_LEN		5
  *Note: Length can be 3, 4, 5
  *	  If Automatic ack is enabled and if mode is PTX, this address is used to set RX_P0 address
  */
#define CONFIG_RFM70_ADDRESS				{0x11, 0x22, 0x33, 0x44, 0x55}
#define CONFIG_RFM70_ADDR_LEN				5


/* Enable or Disable Automatic Retransmit/Acknowledgement feature
 * To enable define to 1, otherwise to 0
 * NOTE: In PTX/PRX, EN_AA=0 for No ack; In PRX/PTX, NO_ACK is checked in the packet
 */
 #define CONFIG_RFM70_AUTOACK_ENABLED 		1
 
 
/* Whether to enable or disable dynamic payload width
 * Define as 1 to enable, 0 to disable
 */
#define CONFIG_RFM70_DYNAMIC_PL_ENABLED		1

/* Length of the static payload
 * Define this from 0 to 32 (bytes)
 */
#define CONFIG_RFM70_STATIC_PL_LENGTH		32


/* Whether to enable Payload in the ACK
   If defined to 1, also define ACK payload length */
#define CONFIG_RFM70_ACK_PL_ENABLED			0
#define CONFIG_RFM70_ACK_PL_LENGTH			4


/* Output power
 * Define this to:
 * 	RFM70_PWR_P5DBM : 5dBm
 * 	RFM70_PWR_0DBM : 0dBm
 * 	RFM70_PWR_M5DBM : -5dBm
 * 	RFM70_PWR_M10DBM : -10dBm
 */
#define CONFIG_RFM70_TX_PWR					RFM70_PWR_P5DBM


/* Data rate
 *	Define this to:
 *		RFM70_RATE_250KBPS
 *		RFM70_RATE_1MBPS
 *		RFM70_RATE_2MBPS
 */
#define CONFIG_RFM70_DATA_RATE				RFM70_RATE_1MBPS


/* Define RF channel */
#define CONFIG_RFM70_RF_CHANNEL 			40


/* Define how many retransmitts that should be performed */
#define CONFIG_RFM70_TX_RETRANSMITS			15


/** 
 * Pipes to enable (EN_RXADDR) - Rx only
   pipes to auto_ack (EN_AA) - Tx and Rx (if EN_AA=0, Enhanced shockburst is disabled)
	each pipe payload width (RX_PW_Px) - Rx only
*/

#endif


