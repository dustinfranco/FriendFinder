
//ECE 3150 LAB 6
//Dustin Franco (dmf89) and Hejia Zhao (hz263)
//TA: Jason
//Code sources:




//DEVICE TWO DETAILS
//ADDRESS IS 1234abcd
//SENDING TO 02000102
//intervals of 250 (parameter passed to sleep function)
//suggested sensitivity value is 650

#include "bsp.h"
#include "mrfi.h"
#include "radios/family1/mrfi_spi.h"
#include <string.h>

/* Useful #defines */
//sensitivity controls how "upright" the device needs to be
#define Sensitivity 650
//a few ports
#define RED_LED 0x01
#define GREEN_LED   0x02
#define BUTTON 0x04

//this int stores the ADC reading and is periodically updated
int reading;
unsigned char status;
/* Function prototypes */
void sleep(unsigned int count);

//volatile because it's in the ISR
volatile int flagone=0;

		//create a packet of the packet class provided
		mrfiPacket_t 	packet;
		//packet message is completely arbitrary
		char msg[] = "PACK\r\n"; 

/* Main function for receive application */
void main(void) 
{
		/* Set a filter address for packets received by the radio
		 *   This should match the "destination" address of
		 *   the packets sent by the transmitter. */
		
			//This is the unique address for the second device
			uint8_t address[] = {0x12,0x34,0xab,0xcd};
		
		
			
			//Packet Creation (from example code provided to class)
			/* First byte of packet frame holds message length in bytes */
			packet.frame[0] = strlen(msg) + 8;	/* Includes 8-byte address header */
			
			/* Next 8 bytes are addresses, 4 each for source and dest. */
			
			//IMPORTANT that the source and destination of the two devices
			//is set properly!
			
			//destination is 02000102
			packet.frame[1] = 0x02;		/* Destination */
			packet.frame[2] = 0x00;
			packet.frame[3] = 0x01;
			packet.frame[4] = 0x02;
			
			//source is 1234abcd
			packet.frame[5] = 0x12;		/* Source */
			packet.frame[6] = 0x34;
			packet.frame[7] = 0xab;
			packet.frame[8] = 0xcd;
			
			/* Remaining bytes are the message/data payload */
			//our packet is just arbitrary since we don't read hte data, it just sends "PACK,"
			strcpy( (char *) &packet.frame[9] , msg );
			
		
			// SETUP COMMANDS:
			//Code copied from link below:
			
			//Discussed in detail in writeup
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
		/* Filter setup return value: success or failure */

		
		//more setup from the radio example code:
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
		/* Set LEDs to output, set green LED high so we know it's device two*/
		P1DIR = RED_LED | GREEN_LED;
		P1OUT = GREEN_LED;
		
		/* Turn on the radio receiver */
		MRFI_RxOn();
		//set up the button
		//enable the pullup transistor and set hte button to input
		P1REN |= BUTTON;   
		P1OUT |= BUTTON;    
		//crank up those phat interrupts
		__bis_SR_register(GIE);

///////////////////
//MAIN WHILE LOOP//
///////////////////		
while(1)
{
		
	int i;
	//wait/repeated sending
	//reset radio receiveflag
	flagone=0;
	//read what's in the ADC memory register
	reading = ADC10MEM;
	//if the button is pressed AND the reading is larger than our sensitivity (defined at the top)
	if((!(P1IN & BUTTON))& reading>Sensitivity)
	{
		//let the user know that the parameters are met
		P1OUT^=0x01;
		for(i=0;i<30;i++)
		{
			//delay set different from device one
			sleep(150);
			//device two is-> if you receive a message, begin transmitting.
			if(flagone)//if for "is the other guy broadcasting?
			{
				MRFI_Transmit(&packet , MRFI_TX_TYPE_FORCED);
			}
		}
		//check the flag
		if(flagone)
		{	
			//if both flags are set at the end of the waiting, blink the green LED 
			//this green blinking LED lets the user know that both devices are transmitting
			P1OUT^=GREEN_LED;
		}
	}
}



/////////////////////
//Radio receive ISR//
/////////////////////

/*mostly based on the example provided*/
void MRFI_RxCompleteISR(void) {
	//Interrupt Service Routine, simply switching a flag
	//IMPORTANT
	//we might be able to forego these two lines:
	mrfiPacket_t	packet;
	MRFI_Receive(&packet);
	//we just use a ping, set
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
