// SpaceInvaders.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the ECE319K Lab 10

// Last Modified: 1/2/2023 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php

// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// buttons connected to PE0-PE3
// 32*R resistor DAC bit 0 on PB0 (least significant bit)
// 16*R resistor DAC bit 1 on PB1
// 8*R resistor DAC bit 2 on PB2 
// 4*R resistor DAC bit 3 on PB3
// 2*R resistor DAC bit 4 on PB4
// 1*R resistor DAC bit 5 on PB5 (most significant bit)
// LED on PD1
// LED on PD0


#include <stdint.h>
#include <stdlib.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ST7735.h"
#include "Random.h"
#include "TExaS.h"
#include "../inc/ADC.h"
#include "Images.h"
#include "../inc/wave.h"
#include "Timer1.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds


int main1(void){
  DisableInterrupts();
  TExaS_Init(NONE);       // Bus clock is 80 MHz 
  Random_Init(1);

  Output_Init();
  ST7735_FillScreen(0x0000);            // set screen to black
  
  ST7735_DrawBitmap(22, 159, PlayerShip0, 18,8); // player ship bottom
  ST7735_DrawBitmap(53, 151, Bunker0, 18,5);
  ST7735_DrawBitmap(42, 159, PlayerShip1, 18,8); // player ship bottom
  ST7735_DrawBitmap(62, 159, PlayerShip2, 18,8); // player ship bottom
  ST7735_DrawBitmap(82, 159, PlayerShip3, 18,8); // player ship bottom

  ST7735_DrawBitmap(0, 9, SmallEnemy10pointA, 16,10);
  ST7735_DrawBitmap(20,9, SmallEnemy10pointB, 16,10);
  ST7735_DrawBitmap(40, 9, SmallEnemy20pointA, 16,10);
  ST7735_DrawBitmap(60, 9, SmallEnemy20pointB, 16,10);
  ST7735_DrawBitmap(80, 9, SmallEnemy30pointA, 16,10);
  ST7735_DrawBitmap(100, 9, SmallEnemy30pointB, 16,10);

  Delay100ms(50);              // delay 5 sec at 80 MHz

  ST7735_FillScreen(0x0000);   // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString("GAME OVER");
  ST7735_SetCursor(1, 2);
  ST7735_OutString("Nice try,");
  ST7735_SetCursor(1, 3);
  ST7735_OutString("Earthling!");
  ST7735_SetCursor(2, 4);
  ST7735_OutUDec(1234);
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
typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[] ="All\x83";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Spanish[]="Adi\xA2s";
const char Goodbye_Portuguese[] = "Tchau";
const char Goodbye_French[] = "Au revoir";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char Language_Portuguese[]="Portugu\x88s";
const char Language_French[]="Fran\x87" "ais";
const char *Phrases[3][4]={
  {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French}
};

// global variables
uint32_t Data;        // 12-bit ADC
uint32_t Position;    // 32-bit fixed-point 0.001 cm
uint32_t ADCMail;			// mailbox for ISR and main program


volatile uint32_t ADCStatus;

void Timer3A_Init(uint32_t period, uint32_t priority){
  volatile uint32_t delay;
  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
  delay = SYSCTL_RCGCTIMER_R;         // user function
  TIMER3_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER3_TAILR_R = period-1;    // 4) reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|(priority<<29); // priority  
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN1_R = 1<<(35-32);      // 9) enable IRQ 35 in NVIC
  TIMER3_CTL_R = 0x00000001;    // 10) enable timer3A
}

void Timer3A_Stop(void){
  NVIC_DIS1_R = 1<<(35-32);   // 9) disable interrupt 35 in NVIC
  TIMER3_CTL_R = 0x00000000;  // 10) disable timer3
}

void Timer3A_Handler(void){
	GPIO_PORTF_DATA_R ^= 0x02;
	Data = ADC_In();
	ADCMail = Data;
	ADCStatus = 1;
  TIMER3_ICR_R = TIMER_ICR_TATOCINT;
}





void PortF_Init(void){
  volatile int delay;
  SYSCTL_RCGCGPIO_R |= 0x20;
  delay = SYSCTL_RCGCGPIO_R;
  GPIO_PORTF_DIR_R |= 0x0E;
  GPIO_PORTF_DEN_R |= 0x0E;
}

// Buttons
void PortE_Init(void){
  volatile int delay;
  SYSCTL_RCGCGPIO_R |= 0x10;
  delay = SYSCTL_RCGCGPIO_R;
  GPIO_PORTE_DIR_R &= ~0x0F; // button input 0-3
  GPIO_PORTE_DEN_R |= 0x0F;	 // digital for 0-3
	GPIO_PORTE_PDR_R |= 0x0F; // enable internal pull down resistor

}

// LED
void PortD_Init(void){
  volatile int delay;
  SYSCTL_RCGCGPIO_R |= 0x08;
  delay = SYSCTL_RCGCGPIO_R;
  GPIO_PORTD_DIR_R |= 0x03; // led output PD0-1
  GPIO_PORTD_DEN_R |= 0x03;	 // digital for PD0-1
}




uint32_t Convert(int32_t x){
	
	if ( ((172 * -x)/4096 + 163) < 0 ){
		return 0;
		
	} else if ( ((172 * -x)/4096 + 163) > 140 ){
		return 140;
		
	} else {
		return ((172 * -x)/4096 + 163);
	}
	
}


struct sprite {
  int32_t x;      // x coordinate
  int32_t y;      // y coordinate
	int32_t oldX;      
  int32_t oldY;
  int32_t vx,vy;  // pixels/30Hz
  const unsigned short *image; // ptr->image
  const unsigned short *black;
  int32_t life;        // dead (0) : alive (1)
  int32_t w; // width
  int32_t h; // height
	
	int32_t blackW; // width
  int32_t blackH; // height
	
  uint32_t needDraw; // true if need to draw
};
typedef struct sprite sprite_t;

 //{playerX, playerY, playerVelX, playerVelY, player, playerBlack, 1, 20, 21, 1};
sprite_t playerSprite; // player is a global variable b/c want other functions to access info



int main(void){ 
	char l;
  DisableInterrupts();
  TExaS_Init(NONE);       		// Bus clock is 80 MHz 
  Output_Init();
	ST7735_InitR(INITR_REDTAB);
	ADC_Init();	
	PortF_Init();	//
	PortE_Init(); // switch 
	PortD_Init(); // LED
	
	Timer3A_Init( 666667, 5); // 10hz = 8000000   : 30hz = 2666667  : 60hz = 1333333  : 120hz 666667
  Timer1_Init(2666667, 5); // sample button at 30Hz
	
	ST7735_FillScreen(0x0000);
	ST7735_SetRotation(3);

	// variables for game
	uint32_t valueOut = 0;
	int32_t playerNewX = 0;
	int32_t playerNewY = 0;
	int32_t playerOldX = 0;
	int32_t playerOldY = 0;
	int32_t playerVelX = 0;			// move right by 1 pixel
	int32_t playerVelY = 0;
	int32_t playerMovedFlag = 0;
	
	
	playerSprite.x = 0;
	playerSprite.y = 128;
	
	playerSprite.oldX = 0;
	playerSprite.oldY = 128;
	
	playerSprite.vx = 1;
	playerSprite.vy = 0;
	playerSprite.image = player;
	playerSprite.black = playerBlack;
	playerSprite.w = 20;
	playerSprite.h = 21;
	
	playerSprite.blackW = 24;
	playerSprite.blackH = 25;
	
	playerSprite.needDraw = 1;
	playerSprite.life = 1;
	
	EnableInterrupts();


	while(1){
		
		if (playerSprite.life){
			
			while (ADCStatus == 0) {}	
			valueOut = ADCMail;
			valueOut = Convert(valueOut);
			
			playerSprite.x = valueOut;
			
			// erase old player if they moved enough
			if ( abs(playerSprite.x - playerSprite.oldX) > 2 || abs(playerSprite.y - playerSprite.oldY) > 2 ){
				ST7735_DrawBitmap(  playerSprite.oldX , playerSprite.oldY , playerSprite.black, playerSprite.blackW , playerSprite.blackH );  // draw new player location
			}

			if ( abs(playerSprite.x - playerSprite.oldX) > 2 ) { // or player y changes
				playerSprite.oldX = 	valueOut;
				ST7735_DrawBitmap(  playerSprite.x , playerSprite.y , playerSprite.image, playerSprite.w, playerSprite.h );
				
			} else {
				ST7735_DrawBitmap(  playerSprite.oldX , playerSprite.y , playerSprite.image, playerSprite.w, playerSprite.h );

			}
			
		}
		
		
		
		ADCStatus = 0;
	}
	
	
	
	
	
	// filler below
	
  for(phrase_t myPhrase=HELLO; myPhrase<= GOODBYE; myPhrase++){
    for(Language_t myL=English; myL<= French; myL++){
         ST7735_OutString((char *)Phrases[LANGUAGE][myL]);
      ST7735_OutChar(' ');
         ST7735_OutString((char *)Phrases[myPhrase][myL]);
      ST7735_OutChar(13);
    }
  }
  Delay100ms(30);
  ST7735_FillScreen(0x0000);       // set screen to black
  l = 128;
  while(1){
    Delay100ms(20);
    for(int j=0; j < 3; j++){
      for(int i=0;i<16;i++){
        ST7735_SetCursor(7*j+0,i);
        ST7735_OutUDec(l);
        ST7735_OutChar(' ');
        ST7735_OutChar(' ');
        ST7735_SetCursor(7*j+4,i);
        ST7735_OutChar(l);
        l++;
      }
    }
  }





  
	
	
	
	
	
	
}

