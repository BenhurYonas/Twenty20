// DAC.h
// Benhur Yonas and Diondre Dubose
// November 25, 2020
// File Header for 6-bit DAC, used in game


// **************DAC_Init*********************
// Initialize 6-bit DAC 
// Input: none
// Output: none
void DAC_Init(void);


// **************DAC_Out*********************
// output to DAC
// Input: 6-bit data, 0 to 15 
// Output: none
void DAC_Out(unsigned long data);
