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

void PortE_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000010;     // 1) E clock
  delay = SYSCTL_RCGC2_R;           // delay   
//  GPIO_PORTE_LOCK_R = 0x4C4F434B;   // 2) unlock PortE PE0  
//  GPIO_PORTE_CR_R = 0x1F;           // allow changes to PE4-0       
  GPIO_PORTE_AMSEL_R &= ~0x03;        // 3) disable analog function
  GPIO_PORTE_PCTL_R &= ~0x00000003;   // 4) GPIO clear bit PCTL  
  GPIO_PORTE_DIR_R &= ~0x03;          // 5) PF4,PF0 input, PF3,PF2,PF1 output   
  GPIO_PORTE_AFSEL_R &= ~0x03;        // 6) no alternate function      
  GPIO_PORTE_DEN_R |= 0x03;          // 7) enable digital pins PF4-PF0     
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
sprite_t Player;
sprite_t Cure;
sprite_t Hornet;

// misc variables used across functions during gameplay
int moveflag, ADCflag; 								// semaphore for needDraw and ADC person movement
int Anyalive; 												// semaphore for end of game
unsigned long ADCdata, ShipDistance; 	// ADC variables used for movement of ship
unsigned long score = 0;							// Initalizes score to zero (currently not implemented)
unsigned long random;									// used to grab value from random generator

// Random250() generates a random number between 0 and 255
unsigned long Random250(void){
	return Random();
	//return ((Random()>>24)%250);
}



// GameInit() initializes all of the sprite data
void GameInit(void){
	moveflag = 0;			// initalize moveflag to zero while sprite data is setup
	
	// Initialize Data for Virus Model  
	Virus.vy = 1;
	Virus.image = COVID_model;
	Virus.blank = white;
	Virus.w = 33;
	Virus.h = 36;
	Virus.needDraw = 0;
	Virus.life = dead;
	
	// Initialize Data for Cure Model 
	Cure.vy = 1;
	Cure.image = test_tube;
	Cure.blank = white;
	Cure.w = 10;
	Cure.h = 18;
	Cure.needDraw = 0;
	Cure.life = dead;
	
	// Initialize Data for Hornet Model  
	Hornet.vx = -1;
	Hornet.image = hornet;
	Hornet.blank = white;
	Hornet.w = 15;
	Hornet.h = 12;
	Hornet.needDraw = 0;
	Hornet.life = dead;
	
	
	// Initialize Data for Player Model (Temporarily SpaceInvaderShip)
	Player.x = 52;
	Player.y = 159;
	Player.image = valvano;
	Player.w = 16;
	Player.h = 20;
	Player.needDraw = 1;
	Player.life = alive;
	Player.blank = white;
	
	Anyalive = 1;  // Initalize player health (potentially add lives in future update)
}


// Collision() detects if two objects are touching each other, registering a collision
// returns a 1 if collision detected
// returns a 0 if no collision detected
// Credit given to Aleksey (http://eekit.blogspot.com/2017/02/the-basics-of-arduino-game-programming_19.html)
unsigned char Collision(unsigned char x1, unsigned char y1, unsigned char w1, unsigned char h1, unsigned char x2, unsigned char y2, unsigned char w2, unsigned char h2){
	//if (x1>(x2+w2) || x2>(x1+w1)) return 0;
	//if (y1>(y2-h2) || y2>(y1-h1)) return 0;
	
	if(((x2+w2)>x1) && ((y2+h2)>y1) && (x2 < (x1+w1)) && (y2 < (y1+h1))){
		return 1;
	}
	return 0;
}




// GameMove() moves the sprites around in relation to the game
// checks for collisions between player and other sprites
// plays sounds based on collisions
void GameMove(void){ 
	
	// Virus model Move (ALPHA)
	if (Virus.life == dead){  		// Spawns new sprite once old one is gone
		random = Random250();
		if (random <= 95){
			Virus.life = alive;
			Virus.x = random;
			Virus.y = 0;
			Virus.needDraw = 1;
		}
	}else if (Virus.life == alive){  // moves sprite down one until it reaches the end
		if(Virus.y > 196){
			Virus.life = dead;					
		}else{
			if(Collision(Player.x, Player.y, Player.w, Player.h, Virus.x, Virus.y, Virus.w, Virus.h) == 1){  // Check for collision, If Collided, dead
				Player.life = dead;
				Anyalive = 0;
			}else{
				Virus.y += Virus.vy;
			}
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
			Cure.life = dead;					 
		}else{
			if(Collision(Player.x, Player.y, Player.w, Player.h, Cure.x, Cure.y, Cure.w, Cure.h) == 1){ // Check for collision, if collided add pt
				Cure.life = dead;
				score++;
				// ADD SOUND HERE
			}else{
				Cure.y += Cure.vy;
			}
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
		if(Hornet.x < -15){
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
		ShipDistance = 111*ADCdata /4096;
		Player.ox = Player.x; 
		Player.x = ShipDistance;
		Player.needDraw = 1;
	} 
	
	
	
	// CHECK FOR JUMP/CROUCH
	long Jumping=5;
if((((GPIO_PORTE_DATA_R)&~0xFFFFFFFC)==0x03)||(((GPIO_PORTE_DATA_R)&~0xFFFFFFFC)==0x01))
	{
		while(Jumping != 0)
			{
				Jumping--;
				Player.y= Player.y +15;
		}
	
}
 if(((GPIO_PORTE_DATA_R)&~0xFFFFFFFC)==0x02)
 {
 }

}

// GameDraw() checks conditions of each sprite and calls ST7735_DrawBitmap() to (re)draw sprites on the LCD
void GameDraw(void){
	
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
		Hornet.needDraw = 0;
	}
	
	
	
	// GameDraw for Player Model
	if(Player.needDraw){
		if(Player.life == alive){
			ST7735_DrawBitmap(Player.ox, Player.y, Player.blank, Player.w, Player.h);
			ST7735_DrawBitmap(Player.x, Player.y, Player.image, Player.w, Player.h);
			
		}else{
			Anyalive = 0;
		}
		Player.needDraw = 0;
		
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
	SysTickInit();
	Random_Init(NVIC_ST_CURRENT_R);
  Output_Init();
	GameInit();
	
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

  //Delay100ms(50);              // delay 5 sec at 80 MHz

  ST7735_FillScreen(0x0000);            // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString("GAME OVER");
  ST7735_SetCursor(1, 2);
	ST7735_OutString("Your Score:");
  ST7735_SetCursor(2, 3);
  LCD_OutUDec(score);
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

