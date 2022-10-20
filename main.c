#include "em_device.h"
#include "em_chip.h"

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



int main(void)
{
  /* Chip errata */
  CHIP_Init();

  SegmentLCD_Init(false);

  int cntr = 0;
  int cntr_tmp = 0;

  // az űrhajó kezdeti pozíciója
  struct spaceship spaceship = {{1, 0}, FORWARD};


  while (1) {
	  if(spaceship.dir == FORWARD) {
		 horizontal(spaceship);
		 spaceship.pos.column = spaceship.pos.column + 1;

		 // ha elérte az űrhajó a pálya végét, akkor a pálya elejéről indul újra
		 if (spaceship.pos.column == SEGMENT_LCD_NUM_OF_LOWER_CHARS)
				 spaceship.pos.column = 0;
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

	 // tesztelés

	 cntr_tmp += 1;

	 if(cntr_tmp == 5)
		 spaceship.dir = UP;
	 if(cntr_tmp == 6)
	 		 spaceship.dir = FORWARD;
	 if(cntr_tmp == 12)
	 		 spaceship.dir = DOWN;
	 if(cntr_tmp == 14)
	 		 spaceship.dir = FORWARD;
	 if(cntr_tmp == 18)
		 	 spaceship.dir = UP;
	 if(cntr_tmp == 20)
		 	 spaceship.dir = FORWARD;
  }
}
