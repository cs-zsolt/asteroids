#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_usart.h"
#include "em_lcd.h"
#include "em_timer.h"
#include "em_gpio.h"

#include "segmentlcd.h"
#include "segmentlcd_individual.h"


SegmentLCD_UpperCharSegments_TypeDef upperCharSegments[SEGMENT_LCD_NUM_OF_UPPER_CHARS];
SegmentLCD_LowerCharSegments_TypeDef lowerCharSegments[SEGMENT_LCD_NUM_OF_LOWER_CHARS];

#define STARTING_SPEED 20000;

volatile bool timerWait = false;
volatile char button;

struct position {
	int row; // 0, 1, 2 sor
	uint16_t column;
};

enum direction {FORWARD, UP, DOWN};
enum button {LEFT, RIGHT};

struct spaceship {
	struct position pos;
	enum direction dir;
};

struct position asteroids[3];


void UART0_RX_IRQHandler() {
	button = USART_Rx(UART0) -32 ;
	USART_Tx(UART0, button);
}

void TIMER0_IRQHandler() {
	timerWait = false;
	TIMER_IntClear(TIMER0, TIMER_IF_OF);
}

void Delay()
{
  while (timerWait);// várakozás, amíg a Timer nem állítja hamisra a timerWait változót
  timerWait = true;
}

// függőleges szegmensek be és kikapcsolása
void changeVerticalSegment(struct spaceship spaceship, bool state) {
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

// vízszintes szegmensek be és kikapcsolása
void changeHorizontalSegment(struct position pos, bool state) {
	switch(pos.row) {
		case 0:
			lowerCharSegments[pos.column].a = state;
			break;
		case 1: // a középső sorban két kis szegmensből áll az űrhajó (g és m)
			lowerCharSegments[pos.column].g = state;
			lowerCharSegments[pos.column].m = state;
			break;
		case 2:
			lowerCharSegments[pos.column].d = state;
			break;
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

// az aszteroidák szegmenseinek be/kikapcsolása
// mivel az aszteroidák csak vízszintesen helyezkedhetnek el,
// így mindhárom aszteroidára meghívjuk a szegmens be/kikapcsoló függvényt
void changeAsteroidsSegment(struct position asteroid[3], bool state) {
	for (int i = 0; i < 3; i++){
		changeHorizontalSegment(asteroid[i], state);
	}
}

void generateRandomAsteroids() {
	srand(TIMER0->CNT);
	do {
		for(int i = 0; i <3; i++) {
				asteroids[i].row = i;
				asteroids[i].column = rand() % 5 + 1;
			}
	} while(asteroids[0].column == asteroids[1].column ||
			  asteroids[0].column == asteroids[2].column ||
			  asteroids[1].column == asteroids[2].column);
}

// tizedespontokat be/kikapcsoló függvény
void changeDPs(bool state) {
	LCD_SegmentSet(LCD_SYMBOL_DP2_COM,LCD_SYMBOL_DP2_SEG, state);
	LCD_SegmentSet(LCD_SYMBOL_DP3_COM,LCD_SYMBOL_DP3_SEG, state);
	LCD_SegmentSet(LCD_SYMBOL_DP4_COM,LCD_SYMBOL_DP4_SEG, state);
	LCD_SegmentSet(LCD_SYMBOL_DP5_COM,LCD_SYMBOL_DP5_SEG, state);
	LCD_SegmentSet(LCD_SYMBOL_DP6_COM,LCD_SYMBOL_DP6_SEG, state);
}


// játék vége függvény
// a tizedespontok villognak, amíg új karakter nem érkezik a soros porton
void gameOver() {
	SegmentLCD_AllOff();
	button = '0';
	while(button == '0') {
		changeDPs(true);
		Delay();
		changeDPs(false);
		Delay();
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

  generateRandomAsteroids();
  changeAsteroidsSegment(asteroids, 1);

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

    int speed = STARTING_SPEED;

    CMU_ClockDivSet(cmuClock_HFPER, cmuClkDiv_1);
    CMU_ClockEnable(cmuClock_TIMER0, true);

    TIMER_Init_TypeDef TIMER0_init = TIMER_INIT_DEFAULT;

    /* f_hfper / (prescaler*(TOP+1) -> 14 Mhz órajel és prescaler=512 --> TOP 27 343 */
    TIMER0_init.prescale = timerPrescale512;

    TIMER_Init(TIMER0, &TIMER0_init);
    TIMER_TopSet(TIMER0, speed);
    TIMER_CounterSet(TIMER0, 0);

    TIMER_IntClear(TIMER0, TIMER_IF_OF);
    TIMER_IntEnable(TIMER0, TIMER_IF_OF);
    NVIC_EnableIRQ(TIMER0_IRQn);

    gameOver();

  while (1) {

	  if(spaceship.dir == FORWARD) {
		 changeHorizontalSegment(spaceship.pos, true);
		 Delay();
		 changeHorizontalSegment(spaceship.pos, false);
		 spaceship.pos.column = spaceship.pos.column + 1;

		 // ha elérte az űrhajó a pálya végét, akkor a pálya elejéről indul újra és gyorsul a játék
		 if (spaceship.pos.column == SEGMENT_LCD_NUM_OF_LOWER_CHARS) {
				 spaceship.pos.column = 0;
				 changeAsteroidsSegment(asteroids, false);
		 	 	 generateRandomAsteroids();
		 	 	 changeAsteroidsSegment(asteroids, true);

		 	 	 // játék gyorsítása
		 	 	 if(speed > 10000) {
		 	 		 speed -= 1000;
		 	 		 TIMER_TopSet(TIMER0, speed);
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
		 changeVerticalSegment(spaceship, true);
		 Delay();
		 changeVerticalSegment(spaceship, false);
	 }

	 else if (spaceship.dir == UP) {
		 switch(spaceship.pos.row) {
		 	case 0: spaceship.pos.row = 1; break;
	   	    case 1: spaceship.pos.row = 0; break;
	   	    case 2: spaceship.pos.row = 1; break;
		 }
		 changeVerticalSegment(spaceship, true);
		 Delay();
		 changeVerticalSegment(spaceship, false);
	 }

	  if(button == 'J') {
		  spaceship.dir = changeDirection(spaceship.dir, RIGHT);
		  button = '0';
	  }
	  else if(button == 'B') {
		  spaceship.dir = changeDirection(spaceship.dir, LEFT);
		  button = '0';
	  }

	  // ellenőrzés, hogy az űrhajó pozíciója egyezik-e valamelyik aszteroida pozíciójával
	  for(int i = 0; i < 3; i++) {
		  if(spaceship.dir == FORWARD && spaceship.pos.row == asteroids[i].row && spaceship.pos.column == asteroids[i].column) {
			  cntr = 0;
			  spaceship.pos.row = 1; // űrhajó visszaállítása az első szegmensre
			  spaceship.pos.column = 0;
			  spaceship.dir = FORWARD;
			  speed = STARTING_SPEED; // kezdeti sebesség visszaállítása
			  TIMER_TopSet(TIMER0, speed);

			  gameOver();
		  	  }
	  	  }
  	  }
}
