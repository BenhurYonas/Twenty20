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
#include "Images.h"
#include "Timer1.h"

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



typedef enum {dead,alive} status_t;
struct sprite{
	long x;   // x-coordinate
	long y;   // y-coordinate
	long vx, vy; //  pix/30Hz
	const unsigned short *image; // pointer to image
	const unsigned short *black;
	status_t life;
	unsigned long w; //width
	unsigned long h; //heigh
	unsigned long needDraw;
};


typedef struct sprite sprite_t;

sprite_t Enemy[18]; 
int Flag; //semaphore for needDraw
int Anyalive; //semaphore for end of game

void GameInit(void){int i;
	Flag = 0;
	for(i=0;i<6;i++){
		Enemy[i].x = 20*i;
		Enemy[i].y = 10;
		Enemy[i].vx = 0;
		Enemy[i].vy = 0;
		Enemy[i].image = SmallEnemy10pointA;
		Enemy[i].black = BlackEnemy;
		Enemy[i].life = alive;
		Enemy[i].w = 16;
		Enemy[i].h = 10;
		Enemy[i].needDraw = 1;
		//Enemy[i].vy = 1;
		//Enemy[i].vx = 1;
		Enemy[i].vy = 1;		
	}

	for(i=7;i<18;i++){
		Enemy[i].life = dead;
	}
}


void GameMove(void){ int i;
	Anyalive = 0;
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
	}
}


void GameDraw(void){int i;
	for(i=0;i<18;i++){
		if(Enemy[i].needDraw){
			if(Enemy[i].life == alive){
				ST7735_DrawBitmap(Enemy[i].x, Enemy[i].y, Enemy[i].image, Enemy[i].w, Enemy[i].h);
			}else{
				ST7735_DrawBitmap(Enemy[i].x, Enemy[i].y, Enemy[i].black, Enemy[i].w, Enemy[i].h);	
			}
			Enemy[i].needDraw = 0;
		}
	}
}

void GameTask(void){ //30Hz
	GameMove();
	
	//check buttons, play sound, check slidepot
	Flag = 1;
}






int main(void){
  DisableInterrupts();
	PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  Random_Init(1);

  Output_Init();
	GameInit();
  ST7735_FillScreen(0x0000);            // set screen to black
  

	ST7735_DrawBitmap(52, 159, PlayerShip0, 18,8); // player ship middle bottom
  ST7735_DrawBitmap(53, 151, Bunker0, 18,5);
	Timer1_Init(&GameTask,80000000/30);
	
	EnableInterrupts();
	do{
		while(Flag==0){};
			Flag = 0;
			GameDraw();
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
