/* *********************************************************
	VS1002 MP3 breakout board test routine. 
	This program contains basic functions to communicate with the
	VS1002 MP3 decoder and play a test tone.
	Owen Osborn, Spark Fun electronics, 10-12-05
  ********************************************************* */

/* *********************************************************
               Function declarations
  ********************************************************* */

void delay(void);
// NOTE: the vs1002 chip is configured in SM_SDISHARE mode.  pg 16 data sheet						
unsigned short int vs1002_SCI_read(unsigned char);
void vs1002_SCI_write(unsigned char, unsigned short int);
// this performs an SDI write (cs inverted). pg 16
void vs1002_sineTest(unsigned char);

// these are specific to the LPC2138 ARM MCU and don't have anything to do with
// the MP3 chip
void Initialize(void);
void feed(void);

void IRQ_Routine (void)   __attribute__ ((interrupt("IRQ")));
void FIQ_Routine (void)   __attribute__ ((interrupt("FIQ")));
void SWI_Routine (void)   __attribute__ ((interrupt("SWI")));
void UNDEF_Routine (void) __attribute__ ((interrupt("UNDEF")));

/**********************************************************
                  Header files
 **********************************************************/
#include "LPC21xx.h"

/**********************************************************
                       MAIN
**********************************************************/

int	main (void) {

    // configure SPI0 pins,   master mode
	PINSEL0	= 0x1500;				// SPI pin connections
    S0SPCCR = 32;              		// SCK = 1 MHz, counter > 8 and even
    S0SPCR  = 0x20;                	// Master, no interrupt enable
	
	IODIR0 |= 0x00000080; 

 
 	IOSET0 |= 0x80000080;   // cs high to start

 	vs1002_SCI_write(0x00, 0x0c20);       	// sets sci_mode register, SM_SDINEW, SM_SDISHARE
 											// SM_TESTS.  pg 25, 26

 	vs1002_sineTest(170);   // test tone frequency (pg 35)
 	
	for (;;) 
	{

	}
}   //main

unsigned short int vs1002_SCI_read(unsigned char address)
{
		unsigned short int temp;
		
		IOCLR0 |= 0x00000080;			// cs low
		
		S0SPDR  = 0x03;                	// read command
   		while (!(S0SPSR & 0x80)) ;     	// wait for transfer completed		
   		
		S0SPDR  = address;            	// address here
   		while (!(S0SPSR & 0x80)) ;     	// wait for transfer completed	
	
		S0SPDR  = 0x00;                
   		while (!(S0SPSR & 0x80)) ;    	// wait for transfer completed
   		temp = S0SPDR	;				// get data here MSBs	
   		
   		temp <<= 8;
   	
		S0SPDR  = 0x00;                
   		while (!(S0SPSR & 0x80)) ;    	// wait for transfer completed	
   		temp += S0SPDR;					// get data here LSBs
   		
   		IOSET0 |= 0x00000080;			// cs high
		
		return temp;
}


void vs1002_SCI_write(unsigned char address, unsigned short int data)
{		
	IOCLR0 |= 0x00000080;			// cs low
	
	S0SPDR  = 0x02;                	// read command
   	while (!(S0SPSR & 0x80)) ;     	// wait for transfer completed		
   	
	S0SPDR  = address;            	// address here
   	while (!(S0SPSR & 0x80)) ;     	// wait for transfer completed	

	S0SPDR  = data >> 8;                
   	while (!(S0SPSR & 0x80)) ;    	// wait for transfer completed

	S0SPDR  = data;                
   	while (!(S0SPSR & 0x80)) ;    	// wait for transfer completed	

   	IOSET0 |= 0x00000080;
		
}

// This is SDI write so cs is reversed (internally inverted), pg 21
// for a sine wave test @ 5168 hz, send sequence: 0x53, 0xEF, 0x6E, 126, 0, 0, 0, 0
void vs1002_sineTest(unsigned char pitch)
{
	IOSET0 |= 0x00000080;			// cs HIGH
	
	S0SPDR  = 0x53;                
   	while (!(S0SPSR & 0x80)) ;     	// wait for transfer completed		
   	
	S0SPDR  = 0xEF;            
   	while (!(S0SPSR & 0x80)) ;     	// wait for transfer completed	

	S0SPDR  = 0x6E;                
   	while (!(S0SPSR & 0x80)) ;    	// wait for transfer completed

	S0SPDR  = pitch;                
   	while (!(S0SPSR & 0x80)) ;    	// wait for transfer completed	
   	
 	S0SPDR  = 0;                
   	while (!(S0SPSR & 0x80)) ;     	// wait for transfer completed		
   	
	S0SPDR  = 0;            
   	while (!(S0SPSR & 0x80)) ;     	// wait for transfer completed	

	S0SPDR  = 0;                
   	while (!(S0SPSR & 0x80)) ;    	// wait for transfer completed

	S0SPDR  = 0;                
   	while (!(S0SPSR & 0x80)) ;    	// wait for transfer completed	

   	IOCLR0 |= 0x00000080;			// cs LOW
}


void delay(void){
	unsigned int count;
	for(count = 0; count < 10000; count++){	
	}
}


/**********************************************************
                      Initialize
**********************************************************/

#define PLOCK 0x400

void Initialize(void)  {
	
 
	// 				Setting the Phased Lock Loop (PLL)
	//               ----------------------------------
	//
	// Olimex LPC-P2106 has a 14.7456 mhz crystal
	//
	// We'd like the LPC2106 to run at 53.2368 mhz (has to be an even multiple of crystal)
	// 
	// According to the Philips LPC2106 manual:   M = cclk / Fosc	where:	M    = PLL multiplier (bits 0-4 of PLLCFG)
	//																		cclk = 53236800 hz
	//																		Fosc = 14745600 hz
	//
	// Solving:	M = 53236800 / 14745600 = 3.6103515625
	//			M = 4 (round up)
	//
	//			Note: M - 1 must be entered into bits 0-4 of PLLCFG (assign 3 to these bits)
	//
	//
	// The Current Controlled Oscilator (CCO) must operate in the range 156 mhz to 320 mhz
	//
	// According to the Philips LPC2106 manual:	Fcco = cclk * 2 * P    where:	Fcco = CCO frequency 
	//																			cclk = 53236800 hz
	//																			P = PLL divisor (bits 5-6 of PLLCFG)
	//
	// Solving:	Fcco = 53236800 * 2 * P
	//			P = 2  (trial value)
	//			Fcco = 53236800 * 2 * 2
	//			Fcc0 = 212947200 hz    (good choice for P since it's within the 156 mhz to 320 mhz range
	//
	// From Table 19 (page 48) of Philips LPC2106 manual    P = 2, PLLCFG bits 5-6 = 1  (assign 1 to these bits)
	//
	// Finally:      PLLCFG = 0  01  00011  =  0x23
	//
	// Final note: to load PLLCFG register, we must use the 0xAA followed 0x55 write sequence to the PLLFEED register
	//             this is done in the short function feed() below
	//
   
	// Setting Multiplier and Divider values
  	PLLCFG=0x23;
  	feed();
  
	// Enabling the PLL */	
	PLLCON=0x1;
	feed();
  
	// Wait for the PLL to lock to set frequency
	while(!(PLLSTAT & PLOCK)) ;
  
	// Connect the PLL as the clock source
	PLLCON=0x3;
	feed();
  
	// Enabling MAM and setting number of clocks used for Flash memory fetch (4 cclks in this case)
	MAMCR=0x2;
	MAMTIM=0x4;
  
	// Setting peripheral Clock (pclk) to System Clock (cclk)
	VPBDIV=0x1;

}


void feed(void)
{
  PLLFEED=0xAA;
  PLLFEED=0x55;
}






/*  Stubs for various interrupts (may be replaced later)  */
/*  ----------------------------------------------------  */

void IRQ_Routine (void) {
	while (1) ;	
}

void FIQ_Routine (void)  {
	while (1) ;	
}


void SWI_Routine (void)  {
	while (1) ;	
}


void UNDEF_Routine (void) {
	while (1) ;	
}










