// Twenty20.c 
// Runs on LM4F120/TM4C123
// Original Author: Jonathan Valvano and Daniel Valvano
// Original Date:   October 11, 2020
// Modified by:     Benhur Yonas and Diondre Dubose
// Modified on:     December 10, 2020


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
 
 
 
// ******* Hardware I/O connections*******************

// Controls:
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PE2/AIN1
// Slide pot pin 3 connected to +3.3V 
// Jump button connected to PE0
// Duck button connected to PE1

// DAC/Sound
// 32*R resistor DAC bit 0 on PB0 (least significant bit)
// 16*R resistor DAC bit 1 on PB1
// 8*R resistor DAC bit 2 on PB2 
// 4*R resistor DAC bit 3 on PB3
// 2*R resistor DAC bit 4 on PB4
// 1*R resistor DAC bit 5 on PB5 (most significant bit)

// LCD Display (using ST7735 display)
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

// PortE_Init() initializes the two buttons used to jump and duck
void PortE_Init(void){     
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


// Four Sprites used in the game
sprite_t Virus;
sprite_t Player;
sprite_t Cure;
sprite_t Hornet;

// misc variables used across functions during gameplay
int moveflag, ADCflag; 								// semaphore for needDraw and ADC person movement
int lives; 														// semaphore for end of game
unsigned long ADCdata, ShipDistance; 	// ADC variables used for movement of ship
unsigned long score = 0;							// Initalizes score to zero (currently not implemented)
unsigned long random;									// used to grab value from random generator
int language; 												// flag for choosing correct language
int jump_flag, duck_flag = 0;					// flag to note if jump is under process

// Random250() generates a random number between 0 and 255
unsigned long Random250(void){
	return Random();
	
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
	Player.vy =-1;
	
	lives = 1;  // Initalize player health (potentially add lives in future update)
	score = 0;
	jump_flag = 0; 
	duck_flag = 0;
}


// Collision() detects if two objects are touching each other, registering a collision
// returns a 1 if collision detected
// returns a 0 if no collision detected
// Credit given to Aleksey (http://eekit.blogspot.com/2017/02/the-basics-of-arduino-game-programming_19.html)
unsigned char Collision(unsigned char x1, unsigned char y1, unsigned char w1, unsigned char h1, unsigned char x2, unsigned char y2, unsigned char w2, unsigned char h2){
	if(((x2+w2)>x1) && ((y2+h2)>y1) && (x2 < (x1+w1)) && (y2 < (y1+h1))){
		return 1;
	}
	return 0;
}




// GameMove() moves the sprites around in relation to the game
// checks for collisions between player and other sprites
// plays sounds based on collisions
void GameMove(void){ 
	
	// Virus model Move 
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
				lives = 0;
			}else{
				Virus.y += Virus.vy;
			}
		}
		Virus.needDraw = 1;
	}
	
	
	// Cure model Move 
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
				Sound_Collected();
			}else{
				Cure.y += Cure.vy;
			}
		}
		Cure.needDraw = 1;
	}
	
		
	// Hornet model Move 
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
			if(Collision(Player.x, Player.y, Player.w, Player.h, Hornet.x, Hornet.y, Hornet.w, Hornet.h) == 1){  // Check for collision, If Collided, dead
				Player.life = dead;
				lives = 0;
			}else{
				Hornet.x += Hornet.vx;	
			}
		}
		Hornet.needDraw = 1;
	}
	
	// Player move 
	Player.ox = Player.x; 
	Player.oy = Player.y;
	if(ADCflag == 1){
		ADCflag = 0;
		ADCdata = ADC0_In();
		ShipDistance = 111*ADCdata /4096;
		Player.x = ShipDistance;
		Player.needDraw = 1;
	} 
	
	// CHECK FOR JUMP/CROUCH
	if((((GPIO_PORTE_DATA_R)&~0xFFFFFFFC)==0x03)||(((GPIO_PORTE_DATA_R)&~0xFFFFFFFC)==0x01)){
		Player.w=26;
		Player.h=27;
		Player.image=valvano_jumping;
		if(Player.y == 159){
			Player.vy = -10;
			Sound_Jump();
		}
		jump_flag = 1;		
	}

	if(jump_flag == 1){
		Player.vy++; 
		if(Player.y > 159){
			Player.vy = 0;
			jump_flag = 0;
			Player.y = 159;
			Player.image = valvano;
			Player.w = 16;
			Player.h = 20;
		}
		Player.y += Player.vy;
	}
	
	if(((GPIO_PORTE_DATA_R)&~0xFFFFFFFC)==0x02){  // Check PE1 to see if squat
		Player.w = 24;
		Player.h = 15;
		Player.image = valvano_squatting;
		Player.y = 159;
		jump_flag = 0;
	}else{
		if(jump_flag == 0){
	
			Player.y = 159;
			Player.image = valvano;
			Player.w = 16;
			Player.h = 20;
		}
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
			ST7735_DrawBitmap(Player.ox, Player.oy, Player.blank, Player.w, Player.h);
			ST7735_DrawBitmap(Player.x, Player.y, Player.image, Player.w, Player.h);
			
		}else{
			lives = 0;
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



// Most Text used in the game
// string[0] is english
// string[1] is spanish

char string_jump[2][20] = {"Left: Jump", "Izquierdo: Salto"};
char string_duck[2][20] = {"Right: Duck", "Derecho: Agacharse"};
char string_score[2][10] = {"Score:", "Marcar:"};
char game_over[2][20] = {"GAME OVER", "JUEGO TERMINADO"};
char your_score[2][20] = {"Your score:","Tu Marcas:"};
char play_again[2][20] = {"Play Again?","Juega de Nuevo?"};
char pushbutton[2][25] = {"Push any Button","Presione el boton"};

int main(void){
  DisableInterrupts();
	
	// Initalize all of the things needed for the game
	PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  Output_Init();
	ADC0_Init();
	PortE_Init();
	Random_Init(1);
	SysTickInit();
	Sound_Init();
	Timer1_Init(&ADC,80000000/40);
	
	
	
	ST7735_FillScreen(0xFFFF);            // set screen to white
	ST7735_SetCursor(4, 4);
	ST7735_OutString("Twenty20");
	 
	ST7735_SetCursor(1, 6);
	ST7735_OutString("English:");
	ST7735_SetCursor(1, 7);
	ST7735_OutString("Left Button");
	
	ST7735_SetCursor(1, 8);
	ST7735_OutString("Espanol:");
	ST7735_SetCursor(1, 9);
	ST7735_OutString("Boton Derecho");
	
	while((GPIO_PORTE_DATA_R&0x03) == 0){}
	if((GPIO_PORTE_DATA_R&0x03) == 0x01){
		language = 0;
	}else{
		if((GPIO_PORTE_DATA_R&0x03) == 0x02){
			language = 1;
		}
	}
	
	while(1){
		GameInit();
		ST7735_FillScreen(0xFFFF);            // set screen to white
		
		ST7735_SetCursor(1, 5);
		ST7735_OutString(string_jump[language]);
		ST7735_SetCursor(1, 7);
		ST7735_OutString(string_duck[language]);
		
		
		
		Delay100ms(20);												// delay 5 sec at 80 MHz
		
		ST7735_FillScreen(0xFFFF);            // set screen to white
		EnableInterrupts();
		
		do{
			ST7735_SetCursor(1, 1);
			ST7735_OutString(string_score[language]);
			ST7735_SetCursor(1, 2);
			LCD_OutUDec(score);
			while(moveflag==0){};
				moveflag = 0;
				GameDraw();
			}
		while(lives);

		DisableInterrupts();
		Delay100ms(10);              				// delay 5 sec at 80 MHz

		ST7735_FillScreen(0xFFFF);            // set screen to white
		ST7735_SetCursor(3, 1);
		ST7735_OutString(game_over[language]);
		ST7735_SetCursor(3, 2);
		ST7735_OutString(your_score[language]);
		ST7735_SetCursor(3, 3);
		LCD_OutUDec(score);
		
		ST7735_SetCursor(1, 5);
		ST7735_OutString(play_again[language]);
		ST7735_SetCursor(1, 6);
		ST7735_OutString(pushbutton[language]);			
		while((GPIO_PORTE_DATA_R&0x03) == 0){}
  }

}



// Delay100ms() delays 100ms 
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

