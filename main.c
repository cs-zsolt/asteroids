#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_usart.h"
#include "em_gpio.h"
#include "em_lcd.h"
#include "time.h"

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

volatile uint32_t msTicks;

void Delay(uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) ; // busy-wait
}

void SysTick_Handler(void)
{
  msTicks++;       /* increment counter necessary in Delay()*/
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


// függőleges irányban halad az űrhajó
void changeVerticalSegment(struct spaceship spaceship, int state) {
	// ha lefele halad, a megfelelő szegmens be/kikapcsolása
	if (spaceship.dir == DOWN) {
		switch(spaceship.pos.row) {
			case 1:
				lowerCharSegments[spaceship.pos.column].f = state;
				break;
			case 2:
				lowerCharSegments[spaceship.pos.column].e = state;
				break;
		}
	}
	// ha felfele halad, a megfelelő szegmens be/kikapcsolása
	else if (spaceship.dir == UP) {
		switch(spaceship.pos.row) {
			case 0:
				lowerCharSegments[spaceship.pos.column].f = state;
				break;
			case 1:
				lowerCharSegments[spaceship.pos.column].e = state;
				break;
		}
	}
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


void changeHorizontalSegment(struct position pos, int state) {
	switch(pos.row) {
		case 0:
			lowerCharSegments[pos.column].a = state;
			break;
		case 1:
			lowerCharSegments[pos.column].g = state;
			lowerCharSegments[pos.column].m = state;
			break;
		case 2:
			lowerCharSegments[pos.column].d = state;
			break;
		}
	 SegmentLCD_LowerSegments(lowerCharSegments);
}


void changeAsteroids(struct position asteroid[3], int state) {
	for (int i = 0; i < 3; i++){
		if(asteroid[i].row == 0) {
		lowerCharSegments[asteroid[i].column].a = state;
		}
		else if(asteroid[i].row == 1) {
		lowerCharSegments[asteroid[i].column].g = state;
		lowerCharSegments[asteroid[i].column].m = state;
		}
		else if(asteroid[i].row == 2) {
		lowerCharSegments[asteroid[i].column].d = state;
		}
	}

	SegmentLCD_LowerSegments(lowerCharSegments);
}

struct position asteroids[3];


void randomgen() {
	srand(msTicks + rand());
	for(int i = 0; i <3; i++) {
		asteroids[i].row = rand() % 3;
		asteroids[i].column = rand() % 5 + 2;
	}

	while((asteroids[0].row == asteroids[1].row && asteroids[1].column == asteroids[1].column) ||
			(asteroids[1].row == asteroids[2].row && asteroids[1].column == asteroids[2].column) ||
			(asteroids[2].row == asteroids[0].row && asteroids[2].column == asteroids[0].column)) {
		for(int i = 0; i <3; i++) {
				asteroids[i].row = rand() % 3;
				asteroids[i].column = rand() % 5 + 2;
			}
	}
}

void changeDPs(bool state) {
	LCD_SegmentSet(LCD_SYMBOL_DP2_COM,LCD_SYMBOL_DP2_SEG, state);
	LCD_SegmentSet(LCD_SYMBOL_DP3_COM,LCD_SYMBOL_DP3_SEG, state);
	LCD_SegmentSet(LCD_SYMBOL_DP4_COM,LCD_SYMBOL_DP4_SEG, state);
	LCD_SegmentSet(LCD_SYMBOL_DP5_COM,LCD_SYMBOL_DP5_SEG, state);
	LCD_SegmentSet(LCD_SYMBOL_DP6_COM,LCD_SYMBOL_DP6_SEG, state);

	Delay(500);
}

volatile char button;

void UART0_RX_IRQHandler() {
	button = USART_Rx(UART0) -32 ;
	USART_Tx(UART0, button);
}

void suspectGame() {
	SegmentLCD_AllOff();
	button = '0';
	while(button == '0') {
		changeDPs(true);
		changeDPs(false);
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
  changeAsteroids(asteroids, 1);

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

    if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) {
        while (1) ;
      }

    int speed = 700;

  while (1) {

	  if(spaceship.dir == FORWARD) {
		 changeHorizontalSegment(spaceship.pos, 1);
		 Delay(speed);
		 changeHorizontalSegment(spaceship.pos, 0);
		 spaceship.pos.column = spaceship.pos.column + 1;

		 // ha elérte az űrhajó a pálya végét, akkor a pálya elejéről indul újra
		 if (spaceship.pos.column == SEGMENT_LCD_NUM_OF_LOWER_CHARS) {
				 spaceship.pos.column = 0;
				 changeAsteroids(asteroids, 0);
		 	 	 randomgen();
		 	 	 changeAsteroids(asteroids, 1);
		 	 	 if(speed > 400) {
		 	 		 speed -= 30;
		 	 	 }
		 	 	cntr++;
		 	 	SegmentLCD_Number(cntr);
		 }
	 }

	 else if(spaceship.dir == DOWN) {
		 switch(spaceship.pos.row) {
		  case 0: spaceship.pos.row = 1; break;
		  case 1: spaceship.pos.row = 2; break;
		  case 2: spaceship.pos.row = 1; break;
		 }
		 changeVerticalSegment(spaceship, 1);
		 Delay(speed);
		 changeVerticalSegment(spaceship, 0);
	 }

	 else if (spaceship.dir == UP) {
		 switch(spaceship.pos.row) {
		 	case 0: spaceship.pos.row = 1; break;
	   	    case 1: spaceship.pos.row = 0; break;
	   	    case 2: spaceship.pos.row = 1; break;
		 }
		 changeVerticalSegment(spaceship, 1);
		 Delay(speed);
		 changeVerticalSegment(spaceship, 0);
	 }

	  if(button == 'J') {
		  spaceship.dir = changeDirection(spaceship.dir, RIGHT);
		  button = '0';
	  }
	  else if(button == 'B') {
		  spaceship.dir = changeDirection(spaceship.dir, LEFT);
		  button = '0';
	  }


	  for(int i = 0; i < 3; i++) {
		  if(spaceship.dir == FORWARD && spaceship.pos.row == asteroids[i].row && spaceship.pos.column == asteroids[i].column) {
			  cntr = 0;
			  spaceship.pos.row = 1;
			  spaceship.pos.column = 0;
			  spaceship.dir = FORWARD;
			  speed = 700;

			  suspectGame();
		  	  }
	  	  }
  	  }
}
