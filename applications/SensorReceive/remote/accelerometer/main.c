#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include "rfm70.h"
#include "mpu6050.h"

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

typedef struct 
{
	uint16_t counter;
	uint8_t acc_x;
	uint8_t acc_y;
	uint8_t acc_z;
	uint8_t gyr_x;
	uint8_t gyr_y;
	uint8_t gyr_z;
} sensor_pkt_t;

static void AVR_Init(void);
static void timer2_async_init(void);

static volatile bool timer_flag;
//static uint8_t tx_buf[CONFIG_RFM70_STATIC_PL_LENGTH];
static uint8_t sensor_data[14];
static uint8_t addr[CONFIG_RFM70_ADDR_LEN] = CONFIG_RFM70_ADDRESS;
static sensor_pkt_t sensor_pkt;

int main(void)
{
	AVR_Init();
	set_sleep_mode(SLEEP_MODE_IDLE);  /* POWER_SAVE mode problem: 32.768kHz crystal not working */
	sleep_mode();  /* Go to sleep */

	while(1)
	{
		if(timer_flag == true) {
			timer_flag = false;
			/* Get MPU6050 Accelerometer data */
			if(0 != mpu6050_get_data(sensor_data, sizeof(sensor_data))) {
				RED_LED_ON();
				while(1);
			}
			else {
				sensor_pkt.acc_x = sensor_data[0];
				sensor_pkt.acc_y = sensor_data[2];
				sensor_pkt.acc_z = sensor_data[4];
				sensor_pkt.gyr_x = sensor_data[8];
				sensor_pkt.gyr_y = sensor_data[10];
				sensor_pkt.gyr_z = sensor_data[12];
				
				//RED_LED_ON();
				rfm70_tx_mode();
				if(0 == rfm70_transmit_packet((uint8_t *)&sensor_pkt, sizeof(sensor_pkt))) {
					//RED_LED_TOGGLE();
				}
				//RED_LED_TOGGLE();
				rfm70_powerdown();
				sensor_pkt.counter++;
			}
		}
		if(BUTTON_PRESSED()) {
			wdt_enable(WDTO_500MS);
			while(1);
		}
		sleep_mode();
	}
}



static void AVR_Init(void)
{
	RED_LED_INIT();
	//RED_LED_ON();
	_delay_ms(200); /* Power on delay */
	//RED_LED_OFF();
	sei();   /* enable interrupts globally (MPU6050 (I2C) requires interrupt) */
	rfm70_init(RFM70_MODE_PTX, addr);	/* Initialize RFM70 in Tx mode */
	if(0 != mpu6050_init(MPU6050_LOW_POWER_ACCEL_MODE)) {
		RED_LED_ON();
		while(1);
	}
	timer2_async_init();
	RED_LED_ON();
	_delay_ms(200);
	RED_LED_OFF();
}

/* Starts Timer2 with asynchronous clock */
static void timer2_async_init(void) 
{                                
#if 0	/* Use Timer 0 */
	TCCR0 = 0x5; 		/* Start timer0 at frequecy = 8MHz/1024 = 128us */
	TIMSK |= (1 << 0); 	/* Enable Interrupt for timer0 overflow */ 
#endif
#if 1  /* Use Timer 2 with I/O clock */
	TIMSK &= ~((1<<TOIE2)|(1<<OCIE2));   //Disable interrupts
	OCR2 = 117;					// 128us * 116 ~= 15ms 
	TCCR2 = (1 << WGM21)|0x07;	// CTC mode for changing timer resolution using OCR2 value; Clock = 8MHz/1024 = 128us;
	TIFR = (1 << OCF2);			// Clear OC interrupt flag
	TIMSK |= (1<<OCIE2);        // Enable OC interrupt
#endif
#if 0  /* Use Timer 2 with Async clock */
	_delay_ms(2000);	//for crystal to become stable
    TIMSK &= ~((1<<TOIE2)|(1<<OCIE2));     //Disable TC0 interrupt
    ASSR |= (1<<AS2);           //set Timer/Counter2 to be asynchronous from the CPU clock 
                                //with a second external clock(32,768kHz)driving it.  
    TCNT2 = 0x00;
	OCR2 = 135;					// Compare value(OCR) to generate 30Hz timer
    TCCR2 = (1 << WGM21)|0x02;	// CTC mode for changing timer resolution using OCR2 value; clock = 32.768kHz/8;
    while(ASSR&0x07);           //Wait until TC0 is updated
	TIFR = (1 << OCF2);
    TIMSK |= (1<<OCIE2);        //set 8-bit Timer/Counter0 Overflow Interrupt Enable                             
#endif
}


ISR(TIMER2_COMP_vect)
{
 	static uint8_t count = 0;	

	count++;
	timer_flag = true;
}


#if 0
ISR(TIMER0_OVF_vect)
{
 	static uint8_t count = 0;
	
	count++;
#if 0
	if(count > 30) /* 1 sec */
	{
		count = 0;
       	timer_flag = true;
	}
#endif
	timer_flag = true;
}
#endif