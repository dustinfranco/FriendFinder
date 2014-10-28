
//ECE 3150 LAB 6
//Dustin Franco (dmf89) and Hejia Zhao (hz263)
//TA: Jason
//Code sources:




//DEVICE ONE DETAILS
//ADDRESS IS 02000102
//SENDING TO 1234abcd
//intervals of 250 (parameter passed to sleep function)
//suggested sensitivity value is 650


#include "bsp.h"
#include "mrfi.h"
#include "radios/family1/mrfi_spi.h"
#include <string.h>
//int to store the reading in
int reading;
//dummy variable
int i;
/* Useful #defines */
//sensitivity controls how "upright" the device needs to be
#define Sensitivity 650
//a few ports
#define RED_LED 0x01
#define GREEN_LED   0x02
#define BUTTON 0x04


/* Function prototypes */
void sleep(unsigned int count);
//declare our flags
volatile int flagone=0;

/* Main function for receive application */
void main(void) {
	
		//PACKET STUFF
		mrfiPacket_t 	packet;
		char msg[] = "PACK\r\n"; 
		
	/* Set a filter address for packets received by the radio
	 *   This should match the "destination" address of
	 *   the packets sent by the transmitter. */
	 
	//This address is different than device two!
	//device one address is 2012
	uint8_t address[] = {0x02,0x00,0x01,0x02};
	
	/* Filter setup return value: success or failure */
	unsigned char status;
	/* Perform board-specific initialization */
	BSP_Init();
	/* Initialize minimal RF interface, wake up radio */
	MRFI_Init();
	MRFI_WakeUp();
	/* Attempt to turn on address filtering
	 *   If unsuccessful, turn on both LEDs and wait forever */
	status = MRFI_SetRxAddrFilter(address);	
	MRFI_EnableRxAddrFilter();
	if( status != 0) {
		P1OUT = RED_LED | GREEN_LED;
		while(1);
	}
	/* First byte of packet frame holds message length in bytes */
	packet.frame[0] = strlen(msg) + 8;	/* Includes 8-byte address header */
	
	
	//IMPORTANT that the source and destination of the two devices
	//is set properly!
	
	/* Next 8 bytes are addresses, 4 each for source and dest. */
	//intended for device with address 1234abcd
	packet.frame[1] = 0x12;		/* Destination */
	packet.frame[2] = 0x34;
	packet.frame[3] = 0xab;
	packet.frame[4] = 0xcd;
	
	//source changed to 02000102
	packet.frame[5] = 0x02;		/* Source */
	packet.frame[6] = 0x00;
	packet.frame[7] = 0x01;
	packet.frame[8] = 0x02;
	
	/* Remaining bytes are the message/data payload */
	strcpy( (char *) &packet.frame[9] , msg );
	
	/* Turn on the radio receiver */
   	MRFI_RxOn();
	//enable pullup
    P1REN |= BUTTON;
	//set button port to input
    P1OUT |= BUTTON;  
	
	// SETUP COMMANDS:
	//Code copied from link below:
			
	//Discussed in detail in writeup
	// SETUP COMMANDS:
		// Set conversion to single channel and continuous-sampling mode
		ADC10CTL1 |= CONSEQ1;
		//Set S/H time, 10-bit converter, and continuous sampling:
		ADC10CTL0 |= ADC10SHT_2 + ADC10ON + MSC;
		//Choose P1.0 (channel 1) as an analog input pin:
		ADC10AE0 |= 1;
		// Start A/D conversion; The result will appear in the memory variable "ADC10MEM"
		ADC10CTL0 |= ADC10SC + ENC;
		// SET VARIABLE n TO THE RESULT OF THE A/D CONVERSION:
		reading = ADC10MEM;
	//turn on interrupts
	__bis_SR_register(GIE);
	while(1)
	{
		//reset flag
		flagone=0;
		//take the reading from the adc register
		reading = ADC10MEM;
		//this just transmits automatically if it's peripherals are met.
			if((!(P1IN & BUTTON))&reading>sensitivity) //conditional if of accel and button
			{
				//blink red LED to let user know peripherals are in proper states
				P1OUT^=0x01;
				
				for(i=0;i<20;i++)
				{
					//sleep interval is different than device two
					sleep(250);
					//transmit and wait to hear back
					MRFI_Transmit(&packet , MRFI_TX_TYPE_FORCED);
				}
				if(flagone==1)
				{
					//blink the green led to signal that all parameters are met
					P1OUT^=GREEN_LED;
				}
			}

	}
}


/* Function to execute upon receiving a packet
 *   Called by the driver when new packet arrives */
void MRFI_RxCompleteISR(void) {
	/* Read the received data packet */
	//we might be able to take this out
	mrfiPacket_t	packet;
	MRFI_Receive(&packet);
	//set the flag
	flagone=1;
}

/* Parameterized "sleep" helper function */
void sleep(unsigned int count) {
	int i;
	for (i = 0; i < 10; i++) {
		while(count > 0) {
			count--;
			__no_operation();
		}
	}	
}