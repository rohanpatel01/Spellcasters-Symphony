// Timer1.c
// Runs on TM4C123
// Use TIMER1 in 32-bit periodic mode to request interrupts at a periodic rate
// Daniel Valvano
// Last Modified: 11/15/2021  
// You can use this timer only if you learn how it works

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013
  Program 7.5, example 7.6

 Copyright 2021 by Jonathan W. Valvano, valvano@mail.utexas.edu
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
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"


struct sprite {
  int32_t x;      // x coordinate
  int32_t y;      // y coordinate
	int32_t oldX;      
  int32_t oldY;
  int32_t vy;  // pixels/30Hz
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

extern sprite_t playerSprite;
extern sprite_t playerShootSprite;


// ***************** Timer1_Init ****************
// Activate TIMER1 interrupts to run user task periodically
// Inputs:  period in units (1/clockfreq)
//          priority is 0 (high) to 7 (low)
// Outputs: none
void Timer1_Init(uint32_t period, uint32_t priority){
volatile int delay;
  SYSCTL_RCGCTIMER_R |= 0x02;   // 0) activate TIMER1
  delay = SYSCTL_RCGCTIMER_R;
  TIMER1_CTL_R = 0x00000000;    // 1) disable TIMER1A during setup
  TIMER1_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER1_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R = period-1;    // 4) reload value
  TIMER1_TAPR_R = 0;            // 5) bus clock resolution
  TIMER1_ICR_R = 0x00000001;    // 6) clear TIMER1A timeout flag
  TIMER1_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|(priority<<13); // priority is bits 15,14,13
// interrupts enabled in the main program after all devices initialized
// vector number 37, interrupt number 21
  NVIC_EN0_R = 1<<21;           // 9) enable IRQ 21 in NVIC
  TIMER1_CTL_R = 0x00000001;    // 10) enable TIMER1A
}


void Timer1A_Handler(void){
	TIMER1_ICR_R = 0x00000001;
	
	// variables for shooting
//	static uint32_t lastStateShoot = 0;
//	uint32_t currentStateShoot = GPIO_PORTE_DATA_R & 0x01; // on SW1
//	
//	
//	
//	// variables for jumping
//	static uint32_t lastStateJump = 0;
//	uint32_t currentStateJump = GPIO_PORTE_DATA_R & 0x04; // on SW3
//	
//	static int jumpFlag = 0;
//	int maxPlayerJumpHeight = 60; // tallest b/c subtracting y pos to get higher jump
//	static int reachedMaxHeightFlag = 0;
//	
//	// if player is grounded then check for jump button again and don't move player until
//	if (playerSprite.y == 126){
//		jumpFlag = 0;
//		reachedMaxHeightFlag = 0;
//	}
//	
//	if (currentStateJump != 0 && lastStateJump == 0 && playerSprite.y == 126){
//		GPIO_PORTD_DATA_R ^= 0x02; // toggle led on button input
//		jumpFlag = 1;
//	}
//	
//	if (jumpFlag){
//		
//		if (playerSprite.y > maxPlayerJumpHeight && !reachedMaxHeightFlag){
//			playerSprite.oldY = playerSprite.y;
//			playerSprite.y -= playerSprite.vy;
//			
//		} else if (playerSprite.y <= maxPlayerJumpHeight && !reachedMaxHeightFlag){
//			reachedMaxHeightFlag = 1;
//			
//		} else if (reachedMaxHeightFlag){
//			playerSprite.oldY = playerSprite.y;
//			playerSprite.y += playerSprite.vy;
//		}
//	}
//	
//	
//	if (currentStateShoot != 0 && lastStateShoot == 0){
//		GPIO_PORTD_DATA_R ^= 0x01; // toggle led on button input
//		playerShootSprite.life ^= 1;
//	}
//	
//	
//	
//	lastStateShoot = currentStateShoot;
//	lastStateJump = currentStateJump;
	
	
}


