#define F_CPU 8000000UL
#include <avr/io.h>
#define hSync	5

uint8_t lineCounter;
uint8_t startingV;
uint8_t bugV;
uint8_t bugNext;
uint8_t bugH;
uint8_t playerX;
uint8_t splashed;
uint8_t missileCheck;
uint8_t heroMPixels;

uint8_t missileH;
uint8_t missileV;

uint8_t explode;

uint8_t moveSpeed;
uint8_t bugMask;
uint8_t gameState;			//0 = Ready to play, 1 = Playing, 2 = Game Ovah
uint8_t button;

const uint8_t bugGraphx[] = {
	
	0b11001100,
	0b10101010,
	0b11001100,
	0b11001101,
	0b11101110,
	0b11011100,
	0b01000010,
	0b01000001,

};

uint8_t bugs[6];

void heroMissileOff() {

	missileV = 0;

}

void xPos(uint8_t theSpacing) {

	do {
		__asm__ __volatile__ ("nop");	
	} while(--theSpacing);

}

void gameOver() {

	gameState = 2;
	moveSpeed = 10;			//Taunt player
	heroMissileOff();
	
}

void drawSprite(uint8_t theData) {

	__asm__ __volatile__ ("OUT 0x01, R24");
	__asm__ __volatile__ ("ROR R24");
	__asm__ __volatile__ ("OUT 0x01, R24");
	__asm__ __volatile__ ("ROR R24");
	__asm__ __volatile__ ("OUT 0x01, R24");
	__asm__ __volatile__ ("ROR R24");
	__asm__ __volatile__ ("OUT 0x01, R24");
	__asm__ __volatile__ ("nop");	
	__asm__ __volatile__ ("OUT 0x01, R24");	//Draw 4th pixel again
	
	__asm__ __volatile__ ("ROL R24");		//Mirror universe the pixels
	__asm__ __volatile__ ("OUT 0x01, R24");
	__asm__ __volatile__ ("ROL R24");
	__asm__ __volatile__ ("OUT 0x01, R24");
	__asm__ __volatile__ ("ROL R24");
	__asm__ __volatile__ ("OUT 0x01, R24");
	
	__asm__ __volatile__ ("LSL R24");		//End by outputting black
	__asm__ __volatile__ ("OUT 0x01, R24");

}

void drawPF() {

	switch(VLMCSR) {
		
		case 2:						//Draw the bugs?
			if (PCMSK < 8) {								//Active vertical sprite line?

				uint8_t spriteLine = bugGraphx[PCMSK];		//Puts data in R24

				if (SMCR & 0x02) {
					spriteLine >>= 4;
				}
				else {
					__asm__ __volatile__ ("nop");			//Only need 3 NOPS since a true above eats a jump
					__asm__ __volatile__ ("nop");
					__asm__ __volatile__ ("nop");
				}
				
				xPos(bugH);
				uint8_t check = 0;
				
				for (int bugsAcross = 0 ; bugsAcross < 6 ; bugsAcross++) {
					
					if (bugs[bugsAcross] & bugMask) {
						drawSprite(spriteLine);						
					}
					else {
						drawSprite(0);
						check++;
					}					
					
				}
				
				if (check < 6 && lineCounter == 102) {		//Did aliens breach the barrier?
					gameOver();
				}
				
				PCMSK++;
				
			}
			else {
				if (explode) {
					PORTB = 0b0010;
					explode--;			
				}			
			}

		break;
			
		case 3:						//Draw the SUPER SPACE SHIELD?
			DDRB = 0b0001;							
		break;
			
		case 5:						//Draw the player
			xPos(playerX);
			
			if (gameState & 1) {			
				DDRB = 1;
				xPos(3);			//Our hero is a box
				DDRB = 0;
			}

		break;
			
		case 6:				//Ground
			DDRB = 0b0011;
			PORTB |= SMCR;
			
			//if (SMCR & 0x08) {
				//PORTB = 0b0011;
			//}
			
			//uint8_t bits = 0x80;
			//
			//xPos(20);
			//
			//do {
				//
				//if (score & bits) {
					//drawSprite(0xFF);
				//}
				//else {
					//drawSprite(0x08);
				//}
				//
				//bits >>= 1;
				//
			//} while (bits);

		break;
		
	}

}

void boardStart() {
	
	for (int x = 0 ; x < 6 ; x++) {			//bring the bugs back to life
		bugs[x] = 0b00011111;
	}
	
	if (!(gameState & 1)) {
		startingV = 4;	
	}
	
	bugV = startingV;						//Put them in place
	bugH = 25;
	splashed = 0;
	moveSpeed = 30;
	
}

void lineLogic() {
	
	if (++EICRA & 0x01) {

		if (lineCounter == bugNext && VLMCSR == 2) {				//Time to draw a line of bugs?
			PCMSK = 0;								//Trigger the sprite draw
			bugMask >>= 1;							//Shift to next row of bugs
			if (--DIDR0) {							//If we didn't draw all rows yet, set up next row
				bugNext += 10;
			}
		}
		
		if (lineCounter == missileV) {				//Time to draw the missile?
			heroMPixels = 0;
		}

		switch(++lineCounter) {					//Draw the screen and do logic line-by-line
			
			case 1:
				ADCSRA |= (1 << ADSC);			//Start ADC conversion
				bugMask = 0b01000000;			//Setup mask for analyzing which bugs are left				
				bugNext = bugV;					//What line to start drawing them on				
			break;
			
			case 2:	
				DIDR0 = 6;						//We want 5 lines of aliens
				VLMCSR = 2;							
			break;
			
			case 103:							//Barrier
				VLMCSR = 3;	
				
				if (missileCheck) {				//Check for missile-bug collisions
					uint8_t beamH = bugH + 8;
					if (missileH > beamH) {		//Check H collision
						beamH = missileH - beamH;
						beamH /= 5;
					
						if (!(beamH & 1)) {
							beamH >>= 1;
							if (missileV > bugV) {	//Check V collision
								uint8_t beamV = missileV - (bugV + 8);
								beamV /= 10;
								beamV = 0x10 >> beamV;
								if (bugs[beamH] & beamV) {
									bugs[beamH] -= beamV;
									heroMissileOff();
									//NVMCMD = 0;
									moveSpeed--;
									splashed++;
									explode = 255;
								}
							}		
						}
					}										
				}
					
			break;
			
			case 104:				//Between barrier and hero
				VLMCSR = 4;
				if (splashed == 30) {	//Got 'em all? Move them closer and new wave
					startingV += 4;
					boardStart();
				}
			break;
			
			case 110:				//Hero
				VLMCSR = 5;
				playerX = ADCL >> 1;
				playerX += 6;
				
				if (playerX > 85) {
					playerX = 85;
				}			
					
				if (!(PINB & 0b0100) && button == 0) {	//Check for button press
					PORTB |= 0b0010;
					PORTB = 0;
					button = 10;
					if (gameState & 1) {
						if (missileV == 0) {			//Fire missile (if one isn't already onscreen)
							missileH = playerX + 5;
							missileV = 106;
						}
					}
					else {
						boardStart();
						gameState = 1;
					}					
				}
			break;
			
			case 115:				//Ground
				VLMCSR = 6;
				if (missileV) {					//Missile active? Move it
					missileV -= 2;				
				}
			break;
						
			case 125:
				VLMCSR = 0;				//Done drawing things for this frame
				SMCR |= 0x08;			//VSYNC pulse on
				if (++NVMCMD > moveSpeed) {
					NVMCMD = 0;
					SMCR += 0x02;		//Advance the animation bit
					if (gameState == 1) {	//Bugs only move if game active
						if (WDTCSR & 1) {	//Right?
							if (bugH++ == 32) {
								WDTCSR = 0;
								bugV+= 4;
							}
						}
						else {
							if (bugH-- == 2) {
								WDTCSR = 1;
								bugV += 4;
							}
						}						
					}

				}
			break;
			
			case 128:
				SMCR &= ~0x08;			//VSYNC pulse off
				DIDR0 =  0b00001000;	//Reset this so ADC will work next go-round				
			break;
			
			case 131:					//V back porch done, restart
				lineCounter = 0;
				EICRA = 0;
				ADCSRB++;				//Increment the interlace toggle
				ADCSRB &= 0x01;			//Mask off so we just have the LSB
			break;		
		}		
	}	
}

int main(void) {
	
	CCP = 0xD8;						//Flag so we can write the protected register
	CLKPSR = 0b0000;				//Set NO CPU clock divider on protected register (8MHz)

	ADMUX = 3;
	ADCSRA = 0b10000011;			//Enable ADC

	TCCR0B = 0b00001001;			//No clock divider, normal waveform
	OCR0A = 495;					//Ticks for 15.7kHz line timing
	TIMSK0 = 0x02;					//Use compare A (may use B for sound, not sure)

	boardStart();

	while (1) {
		
		TIFR0 = 0x07;					//Clear counter flags
		
		if ((PINB & 0b0100) && button) {		//Button debounce timer. Doesn't decrement unless you release the button
			button--;
		}
		
		while(!(TIFR0 & 0x02)) {		//Wait for the Counter A match flag to be set
		}

		if (SMCR & 0x08) {				//Vsync bit set?
			DDRB = 0b0010;				//Output to hi z
			xPos(hSync);				//Hsync inside Vsync is a high pulse
			PORTB = 0;					//Set as 1 so going to output will make pixels
			DDRB = 0b0011;
		}
		else {							//Else normal line
			PORTB = 0;
			DDRB = 0b0011;				//Output to pull signal low
			xPos(hSync);				//Hsync low pulse duration
			DDRB = 0b0010;				//Go back to tri-state (black video)
			PORTB = 1;					//Set as 1 so switching DDRB to output will make white-ish pixels
		}
		
		if (EICRA & ADCSRB) {			//Interlace the player and missiles to make.... Graphics :)
			drawPF();
		}
		else {
			if (missileV) {
				if (heroMPixels < 8) {
				
					xPos(missileH);
				
					DDRB |= 0b0011;			//Pixel on
					DDRB &= 0b0010;			//Pixel off

					heroMPixels++;
				
					if (PCMSK < 8) {		//Did the draw the missile on the same line as a bug?
						missileCheck = 1;	//Check this later on when we have more time
					}
				
				}
				else {
					PORTB = 0b0010;
					if (missileV > 12) {
						xPos(missileV - 12);	
					}		
					PORTB = 0b0000;
				}								
			}

		}

		lineLogic();
		
	}
		
}
