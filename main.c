#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_usart.h"
#include "em_gpio.h"

#include "segmentlcd.h"

#include "segmentlcd_individual.h"


SegmentLCD_UpperCharSegments_TypeDef upperCharSegments[SEGMENT_LCD_NUM_OF_UPPER_CHARS];
SegmentLCD_LowerCharSegments_TypeDef lowerCharSegments[SEGMENT_LCD_NUM_OF_LOWER_CHARS];


/*
    --------- 0,a --------     --------- 1,a --------

   |                          |                      |
   |0,f                       |1,f                   |1,b
   |                          |                      |

    --- 0,g --   -- 0,m --   --- 6,g --   -- 1,m --     ---------->

   |                          |                      |
   |0,e                       |1,e                   |1,c
   |                          |                      |

    --------- 0,d --------     --------- 1,d --------
*/

void delay() {
   for(int d=0;d<600000;d++);
}

enum direction {FORWARD, UP, DOWN};
enum button {LEFT, RIGHT};

struct position {
	int row; // 0, 1, 2 sor
	uint16_t column;
};

struct spaceship {
	struct position pos;
	enum direction dir;
};

volatile char button;

// vízszintes írányban halad az űrhajó
void horizontal(struct spaceship spaceship) {

	// adott szegmens bekapcsolása
	setSegment(spaceship.pos);

    delay();

    // adott szegmens kikapcsolása
    resetSegment(spaceship.pos);
}

// függőleges irányban halad az űrhajó
void vertical(struct spaceship spaceship) {
	// ha lefele halad, a megfelelő szegmens bekapcsolása
	if (spaceship.dir == DOWN) {
		switch(spaceship.pos.row) {
			case 1:
				lowerCharSegments[spaceship.pos.column].f = 1;
				break;
			case 2:
				lowerCharSegments[spaceship.pos.column].e = 1;
				break;
		}
	}
	// ha felfele halad, a megfelelő szegmens bekapcsolása
	else if (spaceship.dir == UP) {
		switch(spaceship.pos.row) {
			case 0:
				lowerCharSegments[spaceship.pos.column].f = 1;
				break;
			case 1:
				lowerCharSegments[spaceship.pos.column].e = 1;
				break;
		}
	}
	SegmentLCD_LowerSegments(lowerCharSegments);
	delay();
	// szegmens kikapcsolása
	lowerCharSegments[spaceship.pos.column].f = 0;
	lowerCharSegments[spaceship.pos.column].e = 0;
	SegmentLCD_LowerSegments(lowerCharSegments);

}

// irányváltás
int changeDirection(enum direction direction, enum button button) {
	// ha lefele vagy felfele haladt az űrhajó és a jobb gombot megnyomták, akkor egyenesen halad tovább
	if (direction == UP && button == RIGHT)
		direction = FORWARD;
	else if (direction == DOWN && button == LEFT)
			direction = FORWARD;
	// ha egyenes haladt az űrhajó és a jobb gombot megnyomták, akkor lefele halad tovább
	else if (direction == FORWARD && button == RIGHT)
		direction = DOWN;
	// ha egyenesen haladt az űrhajó és a bal gombot megnyomták, akkor felfele halad tovább
	else if(direction == FORWARD && button == LEFT)
		direction = UP;

	return direction;
}


void setSegment(struct position pos) {
	switch(pos.row) {
		case 0:
			lowerCharSegments[pos.column].a = 1;
			break;
		case 1:
			lowerCharSegments[pos.column].g = 1;
			lowerCharSegments[pos.column].m = 1;
			break;
		case 2:
			lowerCharSegments[pos.column].d = 1;
			break;
		}
	 SegmentLCD_LowerSegments(lowerCharSegments);
}

void resetSegment(struct position pos) {
	switch(pos.row) {
	      	case 0:
	      		lowerCharSegments[pos.column].a = 0;
	      		break;
	      	case 1:
	      		// középen az űrhajó és az akadályok két szegmensből állnak
	      		lowerCharSegments[pos.column].g = 0;
	      		lowerCharSegments[pos.column].m = 0;
	      		break;
	      	case 2:
	      		lowerCharSegments[pos.column].d = 0;
	      		break;
	      	}
	    SegmentLCD_LowerSegments(lowerCharSegments);
}

struct position asteroids(int row, int column) {
	if(row == 0) {
	lowerCharSegments[column].a = 1;
	}
	else if(row == 1) {
	lowerCharSegments[column].g = 1;
	lowerCharSegments[column].m = 1;
	}
	else if(row == 2) {
	lowerCharSegments[column].d = 1;
	}

	SegmentLCD_LowerSegments(lowerCharSegments);

	struct position pos = {row, column};

	return pos;

}

struct position asteroid1;
struct position asteroid2;
struct position asteroid3;


void randomgen() {

	int row[3];
	int column[3];
	for(int i = 0; i <3; i++) {
		row[i] = rand()%3;
		column[i] = rand()%7;
	}

	for(int i=0; i <2; i++) {
		for(int j=i+1; j<3; j++) {
			if(row[i] == row[j] && column[i] == column[j]) {
				row[j] = rand()%7;
				column[j] = rand()%3;
			}
		}
	}

	asteroid1 = asteroids(row[0], column[0]);
	asteroid2 = asteroids(row[1], column[1]);
	asteroid3 = asteroids(row[2], column[2]);

}

void resetAsteroid(struct position asteroid) {

	if(asteroid.row == 0) {
		lowerCharSegments[asteroid.column].a = 0;
		}
		else if(asteroid.row == 1) {
		lowerCharSegments[asteroid.column].g = 0;
		lowerCharSegments[asteroid.column].m = 0;
		}
		else if(asteroid.row == 2) {
		lowerCharSegments[asteroid.column].d = 0;
		}

		SegmentLCD_LowerSegments(lowerCharSegments);

}

volatile char button;

void UART0_RX_IRQHandler() {
	button = USART_Rx(UART0) -32 ;
	USART_Tx(UART0, button);
}

void gameOver() {
	while(button != 'J' || button != 'B') {
		SegmentLCD_Write('.......');

	}
	button = '0';
	return;
}


int main(void)
{
  /* Chip errata */
  CHIP_Init();

  SegmentLCD_Init(false);

  int cntr = 0;

  // az űrhajó kezdeti pozíciója
  struct spaceship spaceship = {{1, 0}, FORWARD};

  randomgen();

  /***************************************************************/
  USART_InitAsync_TypeDef UART0_init = USART_INITASYNC_DEFAULT;

  /* orajel */
    CMU_ClockEnable(cmuClock_GPIO, true);
    CMU_ClockEnable(cmuClock_UART0, true);

    /* GPIO */
    GPIO_PinModeSet(gpioPortF, 7, gpioModePushPull, 1);
    GPIO_PinModeSet(gpioPortE, 0, gpioModePushPull, 1);
    GPIO_PinModeSet(gpioPortE, 1, gpioModeInput, 0);

    USART_InitAsync(UART0, &UART0_init);

    /* labak kivezetese */
    UART0->ROUTE |= USART_ROUTE_LOCATION_LOC1 | USART_ROUTE_TXPEN | USART_ROUTE_RXPEN;

    USART_IntEnable(UART0, UART_IF_RXDATAV);

    NVIC_EnableIRQ(UART0_RX_IRQn);



  while (1) {

	  if(spaceship.dir == FORWARD) {
		 horizontal(spaceship);
		 spaceship.pos.column = spaceship.pos.column + 1;

		 // ha elérte az űrhajó a pálya végét, akkor a pálya elejéről indul újra
		 if (spaceship.pos.column == SEGMENT_LCD_NUM_OF_LOWER_CHARS) {
				 spaceship.pos.column = 0;
				 resetAsteroid(asteroid1);
				 resetAsteroid(asteroid2);
				 resetAsteroid(asteroid3);
		 	 	 randomgen();
		 }
	 }

	 else if(spaceship.dir == DOWN) {
		 switch(spaceship.pos.row) {
		  case 0: spaceship.pos.row = 1; break;
		  case 1: spaceship.pos.row = 2; break;
		  case 2: spaceship.pos.row = 1; break;
		 }
		 vertical(spaceship);
	 }

	 else if (spaceship.dir == UP) {
		 switch(spaceship.pos.row) {
		 	case 0: spaceship.pos.row = 1; break;
	   	    case 1: spaceship.pos.row = 0; break;
	   	    case 2: spaceship.pos.row = 1; break;
		 }
		 vertical(spaceship);
	 }

	  if(spaceship.pos.column == 0) {
	 		 cntr++;
	 		 SegmentLCD_Number(cntr);
	 	 }

	  if(button == 'J') {
		  spaceship.dir = changeDirection(spaceship.dir, RIGHT);
		  button = '0';
	  }
	  else if(button == 'B') {
		  spaceship.dir = changeDirection(spaceship.dir, LEFT);
		  button = '0';
	  }

	  if((spaceship.pos.row == asteroid1.row && spaceship.pos.column == asteroid1.column) ||
		  (spaceship.pos.row == asteroid2.row && spaceship.pos.column == asteroid2.column) ||
		  (spaceship.pos.row == asteroid2.row && spaceship.pos.column == asteroid2.column)){
		  cntr = 0;

		  gameOver();
	  }
  }
}

