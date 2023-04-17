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
#include <math.h>
#include "../inc/wave.h"
#include "../inc/DAC.h"


struct sprite {
  int32_t x;      // x coordinate
  int32_t y;      // y coordinate
	int32_t oldX;      
  int32_t oldY;
  int32_t vy, vx;  // pixels/30Hz
	int32_t originalVy;
  const unsigned short *image; // ptr->image
  const unsigned short *black;
  int32_t life;        // dead (0) : alive (1)
  int32_t w; // width
  int32_t h; // height
	
	int isPlayer;
	
	int reachedMaxHeightFlag;
	int jumpFlag;
	int shootFlag;
	
	int32_t blackW; // width
  int32_t blackH; // height
	
	struct sprite* shootSpritePointer; // pointer to sprite of shootSprite
	
};
typedef struct sprite sprite_t;

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds
void numToLCD(int num);				// print a number to the LCD
void jumpSprite(sprite_t *sprite1);
int Collision(sprite_t sprite1, sprite_t sprite2);
void shootSprite(sprite_t *sprite);


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

// 4E3A
// Press UP for English
char* english_Selection = "Press UP for English";
char* french_Selection = "Vers le bas francais";

char* Score_English = "Score: ";

char* english_jump_Instruction = "Press UP to jump";
char* english_shoot_Instruction = "Press RIGHT to Shoot";
char* english_float_Instruction = "Hold DOWN to float";
char* english_objective_Instruction = "GL - DON'T DIE";
char* english_continue_Instruction = "UP now to continue";


char* french_jump_Instruction = "Jusqu'\xA0 SAUTER";
char* french_shoot_Instruction = "Droit de TIRRR";
char* french_float_Instruction = "DESCEND pour flotter";
char* french_objective_Instruction = "GLE - ne meurs pas";
char* french_continue_Instruction = "UP pour continuer";

const char *Phrases[3][4]={
  {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French}
};

// global variables
uint32_t Data;        // 12-bit ADC
uint32_t Position;    // 32-bit fixed-point 0.001 cm
uint32_t ADCMail;			// mailbox for ISR and main program
int maxPlayerJumpHeight = 60; // tallest b/c subtracting y pos to get higher jump

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
	
	int value = ((172 * -x)/4096 + 163);
	
	
	if ( value < 0 ){
		return 0;
		
	} else if ( value > 140 ){
		return 140;
		
	} else {
		return (value);
	}
	
}



 //{playerX, playerY, playerVelX, playerVelY, player, playerBlack, 1, 20, 21, 1};
sprite_t playerSprite; // player is a global variable b/c want other functions to access info
sprite_t playerShootSprite;
sprite_t enemySprite;
sprite_t enemyShootSprite;

uint32_t valueOut = 0;
int gameOverFlag = 0;
int enemyKilledFlag = 0;

uint32_t scoreNum = 0;
char scoreNumString;

int languageSelectionFlag = 0;
int tutorialFlag = 0;
int isEnglish = 0;


// go to handler
void Timer3A_Handler(void){
	GPIO_PORTF_DATA_R ^= 0x02;
	Data = ADC_In();
	ADCMail = Data;
	ADCStatus = 1;
	
	playerSprite.oldX = playerSprite.x;
	enemySprite.oldX = enemySprite.x;
	
	valueOut = ADCMail;
	valueOut = Convert(valueOut);
	playerSprite.x = valueOut;
	
	
	// buttons
	static uint32_t lastStateShoot = 0;
	uint32_t currentStateShoot = GPIO_PORTE_DATA_R & 0x01; // on SW1 PE0
	//static int shootFlag = 0;
	
	static uint32_t lastStateJump = 0;
	uint32_t currentStateJump = GPIO_PORTE_DATA_R & 0x04; // on SW3 PE2
	
	static uint32_t lastStateFloat = 0;
	uint32_t currentStateFloat = GPIO_PORTE_DATA_R & 0x08; // on SW4 PE3
	static uint32_t originalVY = 0;
	
	//static int playerJumpFlag = 0;
	static int enemyJump = 0;

	//static int reachedMaxHeightFlag = 0;
	
	
	// values needed for enemy movement
	static int direction = 1; // 1 for right, -1 for left
	static int maxValue = 200;
	static int randomValue = 0; // generates random num from [0, maxValue]
	
	
//	// reset player on grounded
	if (playerSprite.y >= 126){
		playerSprite.oldY = playerSprite.y; // changed
		playerSprite.y = 126; // reground player
		playerSprite.jumpFlag = 0;
		playerSprite.reachedMaxHeightFlag = 0;
	}
	
	// reset enemy on grounded
	if (enemySprite.y >= 126){
		enemySprite.oldY = enemySprite.y; // changed
		enemySprite.y = 126; // reground player
		enemySprite.jumpFlag = 0;
		enemySprite.reachedMaxHeightFlag = 0;
	}
	
	
	
	// finish tutorial
	if (!tutorialFlag && currentStateJump != 0 && lastStateJump == 0 && languageSelectionFlag){
		GPIO_PORTD_DATA_R ^= 0x02;
		tutorialFlag = 1;
	}
	
	// select english
	if (!languageSelectionFlag && currentStateJump != 0 && lastStateJump == 0 ){
		GPIO_PORTD_DATA_R ^= 0x02;
		isEnglish = 1;
		languageSelectionFlag = 1; // turn flag 1 - move past language selection
	}
	
	if (!languageSelectionFlag && currentStateFloat != 0 ){
		GPIO_PORTD_DATA_R ^= 0x02;
		isEnglish = 0; // language is French
		languageSelectionFlag = 1;
	}
	
	if (currentStateJump != 0 && lastStateJump == 0 && playerSprite.y == 126 && tutorialFlag){
		GPIO_PORTD_DATA_R ^= 0x02; // toggle led on button input
		playerSprite.jumpFlag = 1;
		Wave_Jump();
		
	}
	
	if (playerSprite.jumpFlag){
		jumpSprite(&playerSprite);
	}
	
	// shooting code
	// have another flag that only lets you shoot once the previous shot is dead
	
	if (currentStateShoot != 0 && lastStateShoot == 0 && !playerSprite.shootFlag && tutorialFlag){
		GPIO_PORTD_DATA_R ^= 0x01; // toggle led on button input
		(*playerSprite.shootSpritePointer).life = 1;
		playerSprite.shootFlag = 1;
		Wave_Shoot();
		
		// spell spawn location
		(*playerSprite.shootSpritePointer).x = playerSprite.x + playerSprite.w + 2; // spawn fireball infront of player
		(*playerSprite.shootSpritePointer).y = playerSprite.y;
	}
	
	// control fireball when shot
	if (playerSprite.shootFlag){
		// velocity of spell
		shootSprite(&playerSprite);
	}
	
	if (currentStateFloat != 0 && playerSprite.y < 126 && tutorialFlag){ 
		playerSprite.vy = 1;
		
	} else {
		// on release
		playerSprite.vy = playerSprite.originalVy;
	}
	
	// enemy AI
  randomValue = (Random32()>>16) % maxValue; 	// continuously create a new random value

	// enemy randomly changes direction and jumps // from !enemySprite.jumpFlag to see grounded
	if ((randomValue > maxValue - 4) && enemySprite.y == 126 && tutorialFlag ){ 
		enemySprite.vx *= -1;
		enemySprite.jumpFlag = 1;
	}
	
	// enemy jump
	if (enemySprite.jumpFlag){
		jumpSprite(&enemySprite);
	}
	
	// enemy randomly shoots
	randomValue = (Random32()>>16) % maxValue;
	if ((randomValue > maxValue - 4) && tutorialFlag && !enemySprite.shootFlag){
		enemySprite.shootFlag = 1;
		(*enemySprite.shootSpritePointer).life = 1;
//		Wave_Shoot();
	
		(*enemySprite.shootSpritePointer).x = enemySprite.x - enemySprite.w - 2; // spawn fireball infront of player
		(*enemySprite.shootSpritePointer).y = enemySprite.y;
	}
	
	// enemy shoot
	if (enemySprite.shootFlag){
		shootSprite(&enemySprite);
	}
	
	// restrict enemy
	// (160 - enemySprite.w)
	if ( !( (enemySprite.x + enemySprite.vx) > 128 ) && ( !( (enemySprite.x + enemySprite.vx) < enemySprite.w)) && tutorialFlag){
		enemySprite.x += enemySprite.vx;
	
		// make enemy stay a bit back
	} else if (enemySprite.x >= 128 || enemySprite.x <= 30 ){ // if player is at edge immeditely move different direction
		enemySprite.vx *= -1;
		
	}
	
	// detect collision between player and enemy --> end game if true
	if ( Collision(playerSprite, enemySprite) || Collision( (*enemySprite.shootSpritePointer), playerSprite )   ){
		// toggle led if overlap
		GPIO_PORTD_DATA_R ^= 0x01;
		gameOverFlag = 1;
	}

	if (Collision( (*playerSprite.shootSpritePointer), enemySprite)){
		GPIO_PORTD_DATA_R ^= 0x01;
		enemyKilledFlag = 1;
	}
	
	lastStateFloat = currentStateFloat;
	lastStateShoot = currentStateShoot; 
	lastStateJump = currentStateJump;
	
  TIMER3_ICR_R = TIMER_ICR_TATOCINT;
}


int main(void){ 
	char l;
  DisableInterrupts();
  TExaS_Init(NONE);       		// Bus clock is 80 MHz 
  Output_Init();
	ST7735_InitR(INITR_BLACKTAB); //INITR_REDTAB
	ADC_Init();	
	PortF_Init();	
	PortE_Init(); // switch 
	PortD_Init(); // LED
	Wave_Init(); // initializes timer2A for sound and does DAC init
	
	Timer3A_Init( 1333333, 5 ); // 10hz = 8000000   : 30hz = 2666667  : 60hz = 1333333  : 120hz 666667
  Timer1_Init(  1333333, 5 ); // sample button at 30Hz
	
	ST7735_FillScreen(0x0000);
	ST7735_SetRotation(3);
	
	int32_t playerMovedFlag = 0;
	
	// initialize playerShootSprite
	playerShootSprite.x = 0;
	playerShootSprite.y = 0;
	playerShootSprite.oldX = 0;
	playerShootSprite.oldY = 0;
	playerShootSprite.vy = 0;
	playerShootSprite.vx = 2;
	playerShootSprite.image = orange_fireball_small;
	playerShootSprite.black = orange_fireball_small_black;
	playerShootSprite.w = 21;
	playerShootSprite.h = 18;
	playerShootSprite.blackW = 21;
	playerShootSprite.blackH = 18;
	playerShootSprite.life = 0; // life determines if we draw it, starts with not being drawn
	
	// initialize player
	playerSprite.x = 0;
	playerSprite.y = 126;
	playerSprite.oldX = 0;
	playerSprite.oldY = 126; // change
	playerSprite.vy = 7;
	playerSprite.vx = 0;
	playerSprite.originalVy = playerSprite.vy;
	playerSprite.image = player;
	playerSprite.black = playerBlackSame;
	playerSprite.w = 20;
	playerSprite.h = 21;
	playerSprite.blackW = 20; // 24
	playerSprite.blackH = 21; // 25
	playerSprite.life = 1;
	playerSprite.isPlayer = 1;
	
	playerSprite.shootSpritePointer = &playerShootSprite;
	
	enemyShootSprite.x = 0;
	enemyShootSprite.y = 0;
	enemyShootSprite.oldX = 0;
	enemyShootSprite.oldY = 0;
	enemyShootSprite.vy = 0;
	enemyShootSprite.vx = 2;
	enemyShootSprite.image = orange_fireball_small_enemy;
	enemyShootSprite.black = orange_fireball_small_black;
	enemyShootSprite.w = 21;
	enemyShootSprite.h = 18;
	enemyShootSprite.blackW = 21;
	enemyShootSprite.blackH = 18;
	enemyShootSprite.life = 0;

	enemySprite.x = 110;
	enemySprite.y = 126;
	enemySprite.oldX = 80;
	enemySprite.oldY = 126;
	enemySprite.vy = 4;
	enemySprite.vx = 1;
	enemySprite.image = enemy;
	enemySprite.black = blackEnemy;
	enemySprite.w = 25;
	enemySprite.h = 21;
	enemySprite.blackW = 25;
	enemySprite.blackH = 21;
	enemySprite.life = 1;
	enemySprite.isPlayer = 0; // not the player - used for shooting
	
	enemySprite.shootSpritePointer = &enemyShootSprite;
	
	Random_Init(NVIC_ST_CURRENT_R);

	EnableInterrupts();
	
	// language selection
	while (!languageSelectionFlag){
		ST7735_DrawString(1, 3, english_Selection , 255);
		ST7735_DrawString(1, 7, french_Selection , 255);
	}
	
	// title screen for game - create and display image 
	
	ST7735_FillScreen(0x0000);
	Delay100ms(10);
	
	while (!tutorialFlag){
		
		if (isEnglish){
			
			ST7735_DrawString(1, 1, english_jump_Instruction , 255);
			ST7735_DrawString(1, 2, english_shoot_Instruction , 255);
			ST7735_DrawString(1, 3, english_float_Instruction , 255);
			ST7735_DrawString(1, 4, english_objective_Instruction , 255);
			ST7735_DrawString(1, 5, english_continue_Instruction , 255);

			
			// french_continue_Instruction
			
		} else {
			
			ST7735_DrawString(1, 1, french_jump_Instruction , 255);
			ST7735_DrawString(1, 2, french_shoot_Instruction , 255);
			ST7735_DrawString(1, 3, french_float_Instruction , 255);
			ST7735_DrawString(1, 4, french_objective_Instruction , 255);
			ST7735_DrawString(1, 5, french_continue_Instruction , 255);
			
		}
	}
	
	
	ST7735_FillScreen(0x0000);	
	
	// for testing
	tutorialFlag = 1;
	languageSelectionFlag = 1;
	
	while(!gameOverFlag && scoreNum < 10){
		
		// draw score to LCD
		ST7735_DrawString(0, 0, Score_English, 255);
		scoreNumString = (char) (scoreNum + 48);
		ST7735_DrawString(7, 0, &scoreNumString, 255 );
		
		while(!ADCStatus){}
		ADCStatus = 0;
			
		// draw player
		if (playerSprite.life){
			if ( (playerSprite.x != playerSprite.oldX) || (playerSprite.y != playerSprite.oldY) ){
				ST7735_DrawBitmap(  playerSprite.oldX , playerSprite.oldY , playerSprite.black, playerSprite.blackW , playerSprite.blackH );  // draw new player location
				ST7735_DrawBitmap( playerSprite.x , playerSprite.y , playerSprite.image, playerSprite.w, playerSprite.h );
			}
		}
		
		// draw player shot
		if ((*playerSprite.shootSpritePointer).life){
			ST7735_DrawBitmap( (*playerSprite.shootSpritePointer).x , (*playerSprite.shootSpritePointer).y , (*playerSprite.shootSpritePointer).image, (*playerSprite.shootSpritePointer).w, (*playerSprite.shootSpritePointer).h );
		}
		
		// draw enemy
		if (enemySprite.life){
			if ( (enemySprite.x != enemySprite.oldX) || (enemySprite.y != enemySprite.oldY) ){
				ST7735_DrawBitmap( enemySprite.oldX , enemySprite.oldY , enemySprite.black, enemySprite.blackW , enemySprite.blackH );  // draw new player location
				ST7735_DrawBitmap( enemySprite.x , enemySprite.y , enemySprite.image, enemySprite.w, enemySprite.h );
			}
		}
		// draw enemy shot
		if ((*enemySprite.shootSpritePointer).life){
			ST7735_DrawBitmap( (*enemySprite.shootSpritePointer).x , (*enemySprite.shootSpritePointer).y , (*enemySprite.shootSpritePointer).image, (*enemySprite.shootSpritePointer).w, (*enemySprite.shootSpritePointer).h );
		}
		
		if (enemyKilledFlag){
			Wave_Killed();
			ST7735_DrawBitmap(0, 127, smallSkull, 61, 80);
			//Delay100ms(5);
			ST7735_FillScreen(0x0000);
			DisableInterrupts();
			scoreNum++;
			
			enemyKilledFlag = 0;
			
			// reset enemy position
			enemySprite.x = 110;
			enemySprite.y = 126;
			enemySprite.oldX = 80;
			enemySprite.oldY = 126;
			
			// reset starting position for player
			playerSprite.x = 0;
			playerSprite.y = 126;
			playerSprite.oldX = 0;
			playerSprite.oldY = 126;
			
			// remove spells on screen
			(*playerSprite.shootSpritePointer).life = 0;
			(*playerSprite.shootSpritePointer).x = 0;
			(*playerSprite.shootSpritePointer).y = 0;
			
			(*enemySprite.shootSpritePointer).life = 0;
			(*enemySprite.shootSpritePointer).x = 0;
			(*enemySprite.shootSpritePointer).y = 0;
			
			//playerSprite.shootFlag = 1;
			playerSprite.shootFlag = 0; // allow player to shoot
			// also try just making fireball spawn in top right 
			// b/c what could be happening is waiting for fireball to move across screen and reset the flag after
			
			EnableInterrupts();
			
		}
		
	
		ADCStatus = 0;
	
	}
	
	// player died
	if (gameOverFlag){
		Wave_Killed();
		ST7735_FillScreen(0x0000);
		ST7735_DrawBitmap(45, 105, smallSkull, 61, 80);
	}
	
	// win
	if (scoreNum == 10){
		//Wave_Killed();
	}
	
	
}


int Collision(sprite_t sprite1, sprite_t sprite2){
	
	// a bit rought calculations since +- pixel won't affect game
	int sprite1X1 = sprite1.x;
	int sprite1Y1 = sprite1.y;
	
	int sprite1X2 = sprite1.x + sprite1.w;
	int sprite1Y2 = sprite1.y - sprite1.h;
	
	
	int sprite2X1 = sprite2.x;
	int sprite2Y1 = sprite2.y;
	
	int sprite2X2 = sprite2.x + sprite2.w;
	int sprite2Y2 = sprite2.y - sprite2.h;
	
	return !(sprite1X1 > sprite2X2 || sprite1X2 < sprite2X1 || sprite1Y1 < sprite2Y2 || sprite1Y2 > sprite2Y1);
	
}

void jumpSprite(sprite_t *sprite1){
	
	if ((*sprite1).y > maxPlayerJumpHeight && !(*sprite1).reachedMaxHeightFlag){
			(*sprite1).oldY = (*sprite1).y;
			(*sprite1).y -= (*sprite1).vy;
			
		} else if ((*sprite1).y <= maxPlayerJumpHeight && !(*sprite1).reachedMaxHeightFlag){
			(*sprite1).reachedMaxHeightFlag = 1;
			(*sprite1).oldY = (*sprite1).y;
			
		} else if ((*sprite1).reachedMaxHeightFlag){
			(*sprite1).oldY = (*sprite1).y;
			(*sprite1).y += (*sprite1).vy;
		}
}

void shootSprite(sprite_t *sprite){	
	
	if ((*sprite).isPlayer){ // player shoots to the right
		(*(*sprite).shootSpritePointer).x += (*(*sprite).shootSpritePointer).vx;
		// erase fireball if gone off screen
		if ( (*(*sprite).shootSpritePointer).x > 160 + (*(*sprite).shootSpritePointer).w){ 
			(*(*sprite).shootSpritePointer).life = 0;
			(*sprite).shootFlag = 0;
		}
		
	} else { // enemy shoots to the left
		(*(*sprite).shootSpritePointer).x -= (*(*sprite).shootSpritePointer).vx;
		// erase fireball if gone off screen
		if ((*(*sprite).shootSpritePointer).x < 0 - (*(*sprite).shootSpritePointer).w){
			(*(*sprite).shootSpritePointer).life = 0;
			(*sprite).shootFlag = 0;
		}
	}
	
}







