// SpaceInvaders.c
// Runs on LM4F120/TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EdX Lab 15

// Last Modified: 8/11/2020 
// for images and sounds, see http://users.ece.utexas.edu/~valvano/Volume1/Lab10Files
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2020

   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2020

 Copyright 2020 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PE2/AIN1
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "Random.h"
#include "PLL.h"
#include "ADC.h"
#include "DAC.h"
#include "Images.h"
#include "Timer1.h"
#include "Sound.h"



void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds


extern unsigned char String[12];
void ConvertUDec(unsigned long n);
void ConvertDistance(unsigned long n);
void LCD_OutUDec(unsigned long n){
  ConvertUDec(n);     // convert using your function
  ST7735_OutString((char *)String);  // output using your function
}
void LCD_OutDistance(unsigned long n){
  ConvertDistance(n);      // convert using your function
  ST7735_OutString((char *)String);  // output using your function
}

void SysTickInit(void){
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_CURRENT_R = 0;
	NVIC_ST_RELOAD_R = 80000000/30; // 30Hz gameplay
	NVIC_ST_CTRL_R = 0x7;
}


typedef enum {dead,alive} status_t;
struct sprite{
	long x, ox;   // x-coordinate
	long y, oy;   // y-coordinate
	long vx, vy; //  pix/30Hz
	const unsigned short *imageA; // pointer to image
	const unsigned short *imageB;
	const unsigned short *black;
	status_t life;
	unsigned long w; //width
	unsigned long h; //heigh
	unsigned long needDraw;
};
typedef struct sprite sprite_t;

sprite_t Enemy[18]; 
sprite_t Ship;
int move_flag, ADCflag; //semaphore for needDraw and ADC person movement
int Anyalive; //semaphore for end of game
unsigned long ADCdata, ShipDistance; //ADC vars used for movement of ship


void GameInit(void){int i;
	move_flag = 0;
	for(i=0;i<6;i++){
		Enemy[i].x = 20*i;
		Enemy[i].y = 10;
		Enemy[i].vx = 0;
		Enemy[i].vy = 0;
		Enemy[i].imageA = SmallEnemy10pointA;
		//Enemy[i].imageB = SmallEnemy10pointB;
		Enemy[i].black = BlackEnemy;
		Enemy[i].life = alive;
		Enemy[i].w = 16;
		Enemy[i].h = 10;
		Enemy[i].needDraw = 1;
		Enemy[i].vx = 1;
		Enemy[i].vy = 1;		
	}

	for(i=7;i<18;i++){
		Enemy[i].life = dead;
	}
	Ship.x = 52;
	Ship.y = 159;
	Ship.imageA = PlayerShip0;
	Ship.w = 18;
	Ship.h = 8;
	Ship.needDraw = 1;
	Ship.life = alive;
	Ship.black = BlackEnemy;
}


void GameMove(void){ int i;
	Anyalive = 0;
	
	
	//Enemy move (PRE-ALPHA TESTING)
	for(i=0;i<18;i++){
		if(Enemy[i].life == alive){
			Enemy[i].needDraw = 1;
			Anyalive = 1;
			
			if(Enemy[i].y>140){
				Enemy[i].life = dead;
			}else{
				if(Enemy[i].y<10){
					Enemy[i].life = dead;
				}else{
					if(Enemy[i].x<0){
						Enemy[i].life = dead;
					}else{
						if(Enemy[i].x>102){
							Enemy[i].life = dead;
						}else{
							Enemy[i].x += Enemy[i].vx;
							Enemy[i].y += Enemy[i].vy;
						}
					}
				}
			}
		}
		//if(Enemy[i].imageA == SmallEnemy10pointB){
		//	Enemy[i].imageA = SmallEnemy10pointA;
		//}else {Enemy[i].imageA = SmallEnemy10pointB;}
	}
	
	
	
	//Spaceship move (PRE-ALPHA TESTING)
		
	if(ADCflag == 1){
		ADCflag = 0;
		ADCdata = ADC0_In();
		ShipDistance = 110*ADCdata /4096;
		Ship.ox = Ship.x; 
		Ship.x = ShipDistance;
		Ship.needDraw = 1;
	}
	
}


void GameDraw(void){int i;
	for(i=0;i<18;i++){
		if(Enemy[i].needDraw){
			if(Enemy[i].life == alive){
				ST7735_DrawBitmap(Enemy[i].x, Enemy[i].y, Enemy[i].imageA, Enemy[i].w, Enemy[i].h);
			}else{
				ST7735_DrawBitmap(Enemy[i].x, Enemy[i].y, Enemy[i].black, Enemy[i].w, Enemy[i].h);	
			}
			Enemy[i].needDraw = 0;
		}
	}
	if(Ship.needDraw){
		ST7735_DrawBitmap(Ship.ox, Ship.y, Ship.black, Ship.w, Ship.h);
		ST7735_DrawBitmap(Ship.x, Ship.y, Ship.imageA, Ship.w, Ship.h);
		Ship.needDraw = 0;
	}
}


void SysTick_Handler(void){  // game handler, 30 Hz
	GameMove();
	//check buttons, play sound, check slidepot 
	move_flag = 1;
}

void ADC(void){ //40 Hz
	ADCflag = 1;
}


unsigned long score = 0;

int main(void){
  DisableInterrupts();
	PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  Random_Init(1);
  Output_Init();
	
	GameInit();
	SysTickInit();
	Timer1_Init(&ADC,80000000/40);
	
	ST7735_SetTextColor(0x0000);
  
	
	ST7735_FillScreen(0xFFFF);            // set screen to black
	//ST7735_DrawBitmap(52, 159, PlayerShip0, 18,8); // player ship middle bottom
  //ST7735_DrawBitmap(53, 151, Bunker0, 18,5);

	EnableInterrupts();
	ADC0_Init();
	do{
		ST7735_SetCursor(1, 1);
		ST7735_OutString("Score:");
		ST7735_SetCursor(1, 2);
		LCD_OutUDec(score);
		while(move_flag==0){};
			move_flag = 0;
			GameDraw();
			score++;
		}
	while(Anyalive);

//  ST7735_DrawBitmap(0, 9, SmallEnemy10pointA, 16,10);
//  ST7735_DrawBitmap(20,9, SmallEnemy10pointB, 16,10);
//  ST7735_DrawBitmap(40, 9, SmallEnemy20pointA, 16,10);
//  ST7735_DrawBitmap(60, 9, SmallEnemy20pointB, 16,10);
//  ST7735_DrawBitmap(80, 9, SmallEnemy30pointA, 16,10);
//  ST7735_DrawBitmap(100, 9, SmallEnemy30pointB, 16,10);

  Delay100ms(50);              // delay 5 sec at 80 MHz

  ST7735_FillScreen(0x0000);            // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString("GAME OVER");
  ST7735_SetCursor(1, 2);
  ST7735_OutString("Nice try,");
  ST7735_SetCursor(1, 3);
  ST7735_OutString("Earthling!");
  ST7735_SetCursor(2, 4);
  LCD_OutUDec(1234);
  while(1){
  }

}


// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}



void PortF_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000020;     // 1) F clock
  delay = SYSCTL_RCGC2_R;           // delay   
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock PortF PF0  
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0       
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog function
  GPIO_PORTF_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL  
  GPIO_PORTF_DIR_R = 0x0E;          // 5) PF4,PF0 input, PF3,PF2,PF1 output   
  GPIO_PORTF_AFSEL_R = 0x00;        // 6) no alternate function
  GPIO_PORTF_PUR_R = 0x11;          // enable pullup resistors on PF4,PF0       
  GPIO_PORTF_DEN_R = 0x1F;          // 7) enable digital pins PF4-PF0        
}






int mainSOUNDTEST(void){
	unsigned long last, now;
	DisableInterrupts();
	PLL_Init(Bus80MHz);
	PortF_Init();
	Sound_Init();
	EnableInterrupts();
	last = GPIO_PORTF_DATA_R&0x11;
	
	
	while(1){
		now = GPIO_PORTF_DATA_R&0x11;
		if((last ==0x11)&&(now!=0x11)){
			Sound_Killed();
			do{
				now = GPIO_PORTF_DATA_R&0x11;
			}while(now != 0x11);
		}
		
	}
	
	
}



