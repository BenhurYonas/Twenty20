// DAC.c
// Benhur Yonas and Diondre Dubose
// November 25, 2020
// Initialization and output for 6-bit DAC, used in game
// DAC uses Port B on TM4C


#include "DAC.h"
#include "tm4c123gh6pm.h"


// **************DAC_Init*********************
// Initialize 6-bit DAC 
// Input: none
// Output: none
void DAC_Init(void){
	volatile unsigned long delay;

	SYSCTL_RCGCGPIO_R |= 0x02; // activate port B
  delay = SYSCTL_RCGCGPIO_R;    // allow time to finish PortB clock activating
       
 
	GPIO_PORTB_DIR_R |= 0x3F;    // make PB5-0 out
	GPIO_PORTB_AFSEL_R = 0x00;
	GPIO_PORTB_DEN_R |= 0x3F;   // enable digital I/O on PB5-0
	GPIO_PORTB_PCTL_R = 0x00;
	GPIO_PORTB_AMSEL_R = 0x00;
	GPIO_PORTB_DR8R_R |= 0x3F;
}


// **************DAC_Out*********************
// output to DAC
// Input: 6-bit data, 0 to 15 
// Output: none
void DAC_Out(unsigned long data){

	GPIO_PORTB_DATA_R = data;
	
}



