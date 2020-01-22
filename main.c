#include <hidef.h>      /* common defines and macros */
#include "derivative.h" /* derivative-specific definitions */
#pragma bool on;
#pragma cplusplus on;
#include "main_asm.h"

#define LCD_DATA PORTK
#define LCD_CTRL PORTK
#define RS 0x01
#define EN 0x02

unsigned char Door_Closing[16] = {'D', 'O', 'O', 'R', ' ', 'C', 'L', 'O', 'S', 'I', 'N', 'G', '.', '.', '.', '.'};
unsigned char Door_Opening[16] = {'D', 'O', 'O', 'R', ' ', 'O', 'P', 'E', 'N', 'I', 'N', 'G', '.', '.', '.', '.'};
unsigned char Current_Floor[15] = {'C', 'U', 'R', 'R', 'E', 'N', 'T', ' ', 'F', 'L', 'O', 'O', 'R', '.', '.'};
unsigned char Go_to_Floor[15] = {'G', 'O', 'I', 'N', 'G', ' ', 'T', 'O', ' ', 'F', 'L', 'O', 'O', 'R', ' '};
unsigned char x;
unsigned char y;
unsigned char i;
unsigned int floor_status; //specify the up or down status
unsigned int current_floor;
unsigned int next_floor;
unsigned char print;
unsigned int min;
unsigned int count;
unsigned int max;
unsigned int entered;
unsigned int Floor_num[4] = {0, 0, 0, 0}; //store the number of floor inside an array, max is {1,2,3,4,1,2,3,4}
unsigned int delay = 500;
unsigned int arrived = 0;
/* List of functions */
#pragma CODE_SEG NON_BANKED /* interrupt section for this module, placement will be in NON_BANKED area*/

/**********************Interrupt**********************/
interrupt void porthISR(void) // 'Interrupt' keyword: tells the C compiler that this function is an ISR
{
  // the Interrupt occurs when PORTH has a falling edge. Also bit 0 of PIFH will be set to 1 if this happens
  if ((PIFH == 0x10) || (PIFH == 0x01))
  {
    //BIT PTH4 AND PTH0
    Floor_num[3] = 4;
    PIFH = 0x11;
    PORTB = 0xFF;
  }
  else if ((PIFH == 0x20) || (PIFH == 0x02))
  {
    if (entered == 1)
    {
      Floor_num[2] = 3;
    }
    else
    {
      if ((floor_status == 1) && (current_floor < 3))
      {
        next_floor = 3;
      }
      else
      {
        Floor_num[2] = 3;
      }
    }
    PORTB = 0xFF;
    PIFH = 0x22;
  } //BIT PTH5 and PTH1

  else if ((PIFH == 0x40) || (PIFH == 0x04))
  {
    if (entered == 1)
    {
      Floor_num[1] = 2;
    }
    else
    {
      if ((floor_status == 1) && (current_floor < 2))
      {
        next_floor = 2;
      }
      else
      {
        Floor_num[1] = 2;
      }
    }
    PORTB = 0xFF;
    PIFH = 0x44;
  } //BIT PTH6 and PTH 2
  else
  {
    Floor_num[0] = 1;
    PORTB = 0xFF;
    PIFH = 0x88;
  } //BIT PTH7 and PTH3

  // the rti instruction is automatically included at the end
}
/* Interrupt Service Routine for /IRQ pin */

//extern void near irqISR(void); // This is when irqISR is defined outside this file

#pragma CODE_SEG DEFAULT /* change code section to DEFAULT (for Small Memory Model, this is $C000) */
// Interrupt Vector Table
typedef void (*near tIsrFunc)(void); // keyword in HCS12 so that the following is in nonbanked (a PPAGE value is not added) memory
const tIsrFunc _vect[] @0xFFCC = {
    // 0xFFCC is the address to store the PORTH interrupt vector
    /* Interrupt table */
    porthISR // PortH Interrupt
};

void timer(unsigned int);
void write_string(unsigned char *);
void MSDelay(unsigned int);
void COMWRT4(unsigned char);
void DATWRT4(unsigned char);
void clearLCD();
void floor_change(unsigned char *, unsigned char);

/**************MAIN*******************************/
void main(void)
{

  //OPEN MAIN
  EnableInterrupts; // asm("cli") // Enables all maskable interrupts
  DDRB = 0xFF;      //MAKE PORTB OUTPUT
  DDRJ |= 0x02;
  PTJ &= ~0x02; //ACTIVATE LED ARRAY ON PORT B
  DDRP |= 0x0F; //
  PTP |= 0x0F;  //TURN OFF 7SEG LED
  DDRA = 0x0F;  //MAKE ROWS INPUT AND COLUMNS OUTPUT
  DDRJ = 0xFF;  //PTJ as output for Dragon12+ LEDs
  PTJ = 0x0;
  DDRH = 0x0;  //Make PTH as input for DIP Switches
               // Setting bit 0 of PORTH as the Interrupt source
  PIEH = 0xFF; // Port H Interrupt Enable Register
               //	---------------------
  PPSH = 0xF0; // Falling edge on the bit 0-3 of PORTH. Rising edge on bit 4-7;
  entered = 0;
  current_floor = 1;
  next_floor = 1;
  //timer(4000);
  while (1)
  {

    find_next_floor();
    MSDelay(10);

    if (arrived == 0)
    {
      arrived = 1;
      timer(1);
      write_string(Door_Opening);
      MSDelay(delay);
      write_string(Door_Closing);
      timer(1);
    }

    if (current_floor == next_floor)
    {
      entered = 1;
      clearLCD();
      floor_change(Current_Floor, current_floor);
      MSDelay(delay);
      clearLCD();
      entered = 0;
    }
    else
    {
      arrived = 0;
      entered = 1;
      clearLCD();
      floor_change(Go_to_Floor, next_floor);
      if (next_floor > current_floor)
      {
        MSDelay((next_floor - current_floor) * delay);
      }
      else
      {
        MSDelay((current_floor - next_floor) * delay);
      }
      clearLCD();

      floor_change(Current_Floor, next_floor);
      MSDelay(delay);
      clearLCD();
      current_floor = next_floor;
      MSDelay(delay);
      entered = 0;
    }
  }
}

int find_next_floor()
{
  if (floor_status == 1)
  {
    for (i = (current_floor - 1); i < 4; i++)
    {
      PORTB = Floor_num[i];
      if (Floor_num[i] != 0)
      {
        PORTB = 0xF0;
        MSDelay(10);
        next_floor = Floor_num[i];
        Floor_num[i] = 0;
        PORTB = next_floor;
        return 0;
      }
    }
    floor_status = 0;
    next_floor = current_floor;
    return 0;
  }
  else if (floor_status == 0)
  {

    for (i = (current_floor - 1); i > 0; i--)
    {
      PORTB = Floor_num[i];
      if (Floor_num[i] != 0)
      {
        PORTB = 0x0F;
        MSDelay(100);
        next_floor = Floor_num[i];
        Floor_num[i] = 0;
        PORTB = next_floor;
        return 0;
      }
    }

    if (Floor_num[0] != 0)
    {
      PORTB = 0x0F;
      MSDelay(100);
      next_floor = Floor_num[0];
      Floor_num[0] = 0;
      PORTB = next_floor;
      return 0;
    }
    floor_status = 1;
    next_floor = current_floor;
    return 0;
  }
}

void write_string(unsigned char char_list[])
{
  clearLCD();
  for (i = 0; i < 16; i++)
  {
    DATWRT4(char_list[i]);
    MSDelay(1);
  }
  return;
}

void floor_change(unsigned char char_list[], unsigned int floor2)
{

  clearLCD();
  for (i = 0; i < 15; i++)
  {
    DATWRT4(char_list[i]);
    //print floor number;
    MSDelay(1);
  }

  if (next_floor == 1)
  {
    DATWRT4('1');
    MSDelay(1);
  }
  else if (next_floor == 2)
  {
    DATWRT4('2');
    MSDelay(1);
  }
  else if (next_floor == 3)
  {
    DATWRT4('3');
    MSDelay(1);
  }
  else if (next_floor == 4)
  {
    DATWRT4('4');
    MSDelay(1);
  }
  return;
}
void COMWRT4(unsigned char command)
{
  unsigned char x;

  x = (command & 0xF0) >> 2;   //shift high nibble to center of byte for Pk5-Pk2
  LCD_DATA = LCD_DATA & ~0x3C; //clear bits Pk5-Pk2
  LCD_DATA = LCD_DATA | x;     //sends high nibble to PORTK
  MSDelay(1);
  LCD_CTRL = LCD_CTRL & ~RS; //set RS to command (RS=0)
  MSDelay(1);
  LCD_CTRL = LCD_CTRL | EN; //rais enable
  MSDelay(5);
  LCD_CTRL = LCD_CTRL & ~EN; //Drop enable to capture command
  MSDelay(1);                //wait

  x = (command & 0x0F) << 2;   // shift low nibble to center of byte for Pk5-Pk2
  LCD_DATA = LCD_DATA & ~0x3C; //clear bits Pk5-Pk2
  LCD_DATA = LCD_DATA | x;     //send low nibble to PORTK
  LCD_CTRL = LCD_CTRL | EN;    //rais enable
  MSDelay(1);
  LCD_CTRL = LCD_CTRL & ~EN; //drop enable to capture command
  MSDelay(1);
}

void clearLCD()
{
  DDRK = 0xFF;
  COMWRT4(0x33); //reset sequence provided by data sheet

  COMWRT4(0x32); //reset sequence provided by data sheet

  COMWRT4(0x28); //Function set to four bit data length
                 //2 line, 5 x 7 dot format

  COMWRT4(0x06); //entry mode set, increment, no shift

  COMWRT4(0x0E); //Display set, disp on, cursor on, blink off

  COMWRT4(0x01); //Clear display

  COMWRT4(0x80); //set start posistion, home position
}
void DATWRT4(unsigned char data) //printing function
{
  unsigned char x;
  x = (data & 0xF0) >> 2;
  LCD_DATA = LCD_DATA & ~0x3C;
  LCD_DATA = LCD_DATA | x;
  MSDelay(1);
  LCD_CTRL = LCD_CTRL | RS;
  MSDelay(1);
  LCD_CTRL = LCD_CTRL | EN;
  MSDelay(1);
  LCD_CTRL = LCD_CTRL & ~EN;
  MSDelay(5);

  x = (data & 0x0F) << 2;
  LCD_DATA = LCD_DATA & ~0x3C;
  LCD_DATA = LCD_DATA | x;
  LCD_CTRL = LCD_CTRL | EN;
  MSDelay(1);
  LCD_CTRL = LCD_CTRL & ~EN;
  MSDelay(1);
}

/**********************SUBROUTINES***********/

void MSDelay(unsigned int itime)
{
  unsigned int i;
  unsigned int j;
  for (i = 0; i < itime; i++)
  {
    asm_mydelay(1);
  }
}

/************************TIMER***************/
void timer(unsigned int sound)
{
  unsigned int Tcount;
  DDRT = DDRT | 0b00100000; // PT5 as output
  PTT = PTT | 0x20;         //make PT5=1
  MSDelay(100);             //change the delay size to see what happens
  PTT = PTT & 0xDF;         //Make PT5=0
  MSDelay(5);               //change delay size....
}
