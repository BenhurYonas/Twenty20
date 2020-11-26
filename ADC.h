// ADC.h
// Benhur Yonas and Diondre Dubose
// November 25, 2020
// File Header for ADC conversion, used in game



//------------ADC0_Init------------
// Initalize ADC0 for ADC collection
// This initialization function sets up the ADC 
// Max sample rate: <=125,000 samples/second
// SS3 triggering event: software trigger
// SS3 1st sample source: channel 1
// SS3 interrupts: enabled but not promoted to controller
// Input: none
// Output: none
void ADC0_Init(void);



//------------ADC0_In------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
unsigned long ADC0_In(void);
