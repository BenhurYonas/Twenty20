// SpaceInvaders.c (RENAME SOON)
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


// UDec and Distance formulas derived from Lab 11; calls Print.s
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

// SysTick Initalization: 30 Hz interrupts
void SysTickInit(void){
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_CURRENT_R = 0;
	NVIC_ST_RELOAD_R = 80000000/30; // 30Hz gameplay
	NVIC_ST_CTRL_R = 0x7;
}

// status of sprite, alive or dead
typedef enum {dead,alive} status_t; 


// Structure of all Sprites used in the game
struct sprite{
	long x, ox;   								// x-coordinate, current and old
	long y, oy;  									// y-coordinate, current and old
	long vx, vy; 									// movement value for x and y directions
	const unsigned short *image; 	// pointer to image
	const unsigned short *imageB; // secondary pointer to image, if needed (may be unnecessary - maybe for flapping hornet wings?)
	const unsigned short *black; 	// antiquated, no need for this
	const unsigned short *blank;	// blank value to clear the screen when sprite disappears from screen (pure white or black)
	status_t life;								// status of sprite, alive or dead
	unsigned long w; 							// width
	unsigned long h; 							// height
	unsigned long needDraw;       // flag set if sprite is moved around 
};

// Create variable based on sprite structure from above
typedef struct sprite sprite_t;

// Antiquated, used for testing purposes
//sprite_t Enemy[18]; 

// Four Sprites used in the game
sprite_t Virus;
sprite_t Ship;
sprite_t Cure;
sprite_t Hornet;

// misc variables used across functions during gameplay
int moveflag, ADCflag; 								// semaphore for needDraw and ADC person movement
int Anyalive; 												// semaphore for end of game
unsigned long ADCdata, ShipDistance; 	// ADC variables used for movement of ship
unsigned long score = 0;							// Initalizes score to zero (currently not implemented)
unsigned long random;													// used to grab value from random generator

// Random250() generates a random number between 0 and 255
unsigned long Random250(void){
	return Random();
	//return ((Random()>>24)%250);
}



// GameInit() initializes all of the sprite data
void GameInit(void){
	moveflag = 0;			// initalize moveflag to zero while sprite data is setup
	
// Antiquated for testing
//	for(i=0;i<6;i++){
//		Enemy[i].x = 20*i;
//		Enemy[i].y = 10;
//		Enemy[i].vx = 0;
//		Enemy[i].vy = 0;
//		Enemy[i].image = SmallEnemy10pointA;
//		//Enemy[i].imageB = SmallEnemy10pointB;
//		Enemy[i].black = BlackEnemy;
//		Enemy[i].life = alive;
//		Enemy[i].w = 16;
//		Enemy[i].h = 10;
//		Enemy[i].needDraw = 1;
//		Enemy[i].vx = 1;
//		Enemy[i].vy = 1;		
//	}

//	for(i=7;i<18;i++){
//		Enemy[i].life = dead;
//	}
	
	
	
	// Initialize Data for Virus Model  (Temporarily SpaceInvaderEnemy10)
	Virus.vy = 1;
	Virus.image = SmallEnemy10pointA;
	Virus.blank = BlackEnemy;
	Virus.w = 16;
	Virus.h = 10;
	Virus.needDraw = 0;
	Virus.life = dead;
	
	// Initialize Data for Cure Model  (Temporarily SpaceInvaderEnemy20)
	Cure.vy = 1;
	Cure.image = SmallEnemy20pointA;
	Cure.blank = BlackEnemy;
	Cure.w = 16;
	Cure.h = 10;
	Cure.needDraw = 0;
	Cure.life = dead;
	
	// Initialize Data for Hornet Model  (Temporarily SpaceInvaderEnemy30)
	Hornet.vx = -1;
	Hornet.image = SmallEnemy30pointA;
	Hornet.blank = BlackEnemy;
	Hornet.w = 16;
	Hornet.h = 10;
	Hornet.needDraw = 0;
	Hornet.life = dead;
	
	
	// Initialize Data for Player Model (Temporarily SpaceInvaderShip)
	Ship.x = 52;
	Ship.y = 159;
	Ship.image = PlayerShip0;
	Ship.w = 18;
	Ship.h = 8;
	Ship.needDraw = 1;
	Ship.life = alive;
	Ship.black = BlackEnemy;
	
	Anyalive = 1;  // Initalize player health (potentially add lives in future update)
}



// GameMove() moves the sprites around in relation to the game, checks for collisions between player and other sprites
void GameMove(void){ 
	
	
//	//Enemy move (PRE-ALPHA TESTING)
//	for(i=0;i<18;i++){
//		if(Enemy[i].life == alive){
//			Enemy[i].needDraw = 1;
//			Anyalive = 1;
//			
//			if(Enemy[i].y>140){
//				Enemy[i].life = dead;
//			}else{
//				if(Enemy[i].y<10){
//					Enemy[i].life = dead;
//				}else{
//					if(Enemy[i].x<0){
//						Enemy[i].life = dead;
//					}else{
//						if(Enemy[i].x>102){
//							Enemy[i].life = dead;
//						}else{
//							Enemy[i].x += Enemy[i].vx;
//							Enemy[i].y += Enemy[i].vy;
//						}
//					}
//				}
//			}
//		}
//		//if(Enemy[i].imageA == SmallEnemy10pointB){
//		//	Enemy[i].imageA = SmallEnemy10pointA;
//		//}else {Enemy[i].imageA = SmallEnemy10pointB;}
//	}
	
	
	
	// Virus model Move (ALPHA)
	if (Virus.life == dead){  		// Spawns new sprite once old one is gone
		random = Random250();
		if (random <= 112){
			Virus.life = alive;
			Virus.x = random;
			Virus.y = 0;
			Virus.needDraw = 1;
		}
	}else if (Virus.life == alive){  // moves sprite down one until it reaches the end
		if(Virus.y > 160){
			Virus.life = dead;					 // Check for collision [NEEDS TO GET IMPLEMENTED] // If Collided, dead
		}else{
			Virus.y += Virus.vy;
		}
		Virus.needDraw = 1;
	}
	
	
	// Cure model Move (ALPHA)
	if (Cure.life == dead){  		// Spawns new sprite once old one is gone
		random = Random250();
		if (random <= 112){
			Cure.life = alive;
			Cure.x = random;
			Cure.y = 0;
			Cure.needDraw = 1;
		}
	}else if (Cure.life == alive){  // moves sprite down one until it reaches the end
		if(Cure.y > 160){
			Cure.life = dead;					 // Check for collision [NEEDS TO GET IMPLEMENTED], if collided add pt
		}else{
			Cure.y += Cure.vy;
		}
		Cure.needDraw = 1;
	}
	
		
	// Hornet model Move (ALPHA)
	if (Hornet.life == dead){  		// Spawns new sprite once old one is gone
		random = Random250();
		if ((random <= 160) && (random > 120)){
			Hornet.life = alive;
			Hornet.x = 143;
			Hornet.y = random;
			Hornet.needDraw = 1;
		}
	}else if (Hornet.life == alive){  // moves sprite down one until it reaches the end
		if(Hornet.x < -16){
			Hornet.life = dead;					 // Check for collision [NEEDS TO GET IMPLEMENTED]; if collided, dead
		}else{
			Hornet.x += Hornet.vx;
		}
		Hornet.needDraw = 1;
	}
	
	
	
	
	// Player move (ALPHA)
		
	if(ADCflag == 1){
		ADCflag = 0;
		ADCdata = ADC0_In();
		ShipDistance = 110*ADCdata /4096;
		Ship.ox = Ship.x; 
		Ship.x = ShipDistance;
		Ship.needDraw = 1;
	} // CHECK FOR JUMP/CROUCH
	
	
	
}


// GameDraw() checks conditions of each sprite and calls ST7735_DrawBitmap() to (re)draw sprites on the LCD
void GameDraw(void){
	
// antiquated
	
//	for(i=0;i<18;i++){
//		if(Enemy[i].needDraw){
//			if(Enemy[i].life == alive){
//				ST7735_DrawBitmap(Enemy[i].x, Enemy[i].y, Enemy[i].image, Enemy[i].w, Enemy[i].h);
//			}else{
//				ST7735_DrawBitmap(Enemy[i].x, Enemy[i].y, Enemy[i].black, Enemy[i].w, Enemy[i].h);	
//			}
//			Enemy[i].needDraw = 0;
//		}
//	}
	
	
	
	// GameDraw for Virus Model
	if(Virus.needDraw){
		if(Virus.life == alive){
			ST7735_DrawBitmap(Virus.x, Virus.y, Virus.image, Virus.w, Virus.h);
		}else{
			ST7735_DrawBitmap(Virus.x, Virus.y, Virus.blank, Virus.w, Virus.h);
		}
		Virus.needDraw = 0;
	}
	
	// GameDraw for Cure Model
	if(Cure.needDraw){
		if(Cure.life == alive){
			ST7735_DrawBitmap(Cure.x, Cure.y, Cure.image, Cure.w, Cure.h);
		}else{
			ST7735_DrawBitmap(Cure.x, Cure.y, Cure.blank, Cure.w, Cure.h);
		}
		Cure.needDraw = 0;
	}
	
	// GameDraw for Hornet Model
	if(Hornet.needDraw){
		if(Hornet.life == alive){
			ST7735_DrawBitmap(Hornet.x, Hornet.y, Hornet.image, Hornet.w, Hornet.h);
		}else{
			ST7735_DrawBitmap(Hornet.x, Hornet.y, Hornet.blank, Hornet.w, Hornet.h);
		}
		Cure.needDraw = 0;
	}
	
	
	
	// GameDraw for Player Model
	if(Ship.needDraw){
		if(Ship.life == alive){
			ST7735_DrawBitmap(Ship.ox, Ship.y, Ship.black, Ship.w, Ship.h);
			ST7735_DrawBitmap(Ship.x, Ship.y, Ship.image, Ship.w, Ship.h);
			
		}else{
			Anyalive = 0;
		}
		Ship.needDraw = 0;
		
	}
}

// SysTick Handler called every 30 Hz, calls GameMove()
void SysTick_Handler(void){  // game handler, 30 Hz
	GameMove();
	//check buttons, play sound, check slidepot 
	moveflag = 1;
}

// ADC Handler called every 40 Hz, sets the ADC flag to sample data from the potentiometer
void ADC(void){ //40 Hz
	ADCflag = 1;
}




int main(void){
  DisableInterrupts();
	PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  
	// FUTURE: Menu screen before initalizing the game
	// would go around this point; Output_Init would have 
	// to get called before here to allow display
	
	
	// Initalize all of the things needed for the game
	Random_Init(1);
  Output_Init();
	GameInit();
	SysTickInit();
	//Sound_Init();
	ADC0_Init();
	Timer1_Init(&ADC,80000000/40);
	
	ST7735_FillScreen(0xFFFF);            // set screen to white
	
	// antiquated testing
	//ST7735_DrawBitmap(52, 159, PlayerShip0, 18,8); // player ship middle bottom
  //ST7735_DrawBitmap(53, 151, Bunker0, 18,5);

	EnableInterrupts();
	
	do{
		ST7735_SetCursor(1, 1);
		ST7735_OutString("Score:");
		ST7735_SetCursor(1, 2);
		LCD_OutUDec(score);
		while(moveflag==0){};
			moveflag = 0;
			GameDraw();
			score++;
			//Sound_Shoot();
		}
	while(Anyalive);

// antiquated testing
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










void PortF_Init(void);
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

