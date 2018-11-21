#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"
#include <pru_ctrl.h>

#define DELAY 100000000

//#define MEMLOC 0xa0000000
#define SIZE 1000
#define ARM_2_PRU 0x00000001
#define PRU_2_ARM 0x00000002
#define BUF 0x00000004
//#define PRU_BASE_ADDR 0x4a300000
//#define PRU_BASE_ADDR 0x00000000
#define PRU_BASE_ADDR 0x90000000
#define STATUS 0x1000
#define STATUS_MEM (PRU_BASE_ADDR + STATUS)
#define BUF0  (PRU_BASE_ADDR + 0x00002000)
#define BUF1  (PRU_BASE_ADDR + 0x00003000)

#define ROWS 975  //rows per image
#define COLS 1280 //pixels per row
#define CELLS COLS/4 //a cell is a 32 bit word hold 4 byte sized pixels //320

#define VSYNC 0x01
#define HSYNC 0x02

void flash(char led);
void dance();
void clear();

volatile register uint32_t __R30;
volatile register uint32_t __R31;

volatile uint32_t LED[] = {
	0x0001, //D1 Blue
	0x0002, //D2 Green
	0x0004, //D3 Orange
	0x0008  //D4 Red
};

void main(void)
{

	// Clear SYSCFG[STANDBY_INIT] to enable OCP master port
	// This is supposed to allow us to write to the global memory space?
	// it works without this, sooo....
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;



	//Apparently there is no difference when parallel capture is used!?!?!
	//Parallel Capture Settings
	CT_CFG.GPCFG0_bit.PRU0_GPI_MODE = 0x00; //enable parallel capture
	CT_CFG.GPCFG0_bit.PRU0_GPI_CLK_MODE = 0x01; //capture on positive edge

	//CT_CFG.GPCFG0_bit.PRU0_GPI_MODE = 0x00; //GPIO direct mode(default)

	volatile int *buf0 = (volatile int *)BUF0;
	volatile int *buf1 = (volatile int *)BUF1;

	//Flash the LEDs a few times at startup
	//dance();
	//clear();

	int *status;
	status = (int*)STATUS_MEM;

	//zero status flags
	*status &= ~ARM_2_PRU;
	*status &= ~PRU_2_ARM;

	volatile int *addr;

	//holds to current buffer to write to
	int curBuf = 0;

	//int *max = (int*)ptr + (SIZE); //making this volatile slows it down alot!!
	int *buf0max = (int*)buf0 + CELLS ; //making this volatile slows it down alot!!
	int *buf1max = (int*)buf1 + CELLS ; //making this volatile slows it down alot!!

	//wait for signal
	while((*status & ARM_2_PRU) < 1) //TODO: need a timeout here 
		__delay_cycles(1); //delay is needed between checking memory??? TODO: Use interupts!
	//Minimum needed delay is just 1. However, more delay give the linux side time to access memory?
	//
	//clear the flag
	*status &= ~ARM_2_PRU;

	int writeReg = 0x00;

	while(1)
	{
		while((__R31 & VSYNC) == 0); //wait for VSYNC to go high
		//while(__R31 & VSYNC) //while VSYNC is high //used to contain below within this loop, not sure which is better. TODO test!

		//TODO should I loop through the number of lines here?
		if(!curBuf) //if on buf0
		{
			//loop through every word in buffer
			for(addr = buf0 ; addr < buf0max ; ++addr)
			{
				//TODO use macro when waiting for clock
				//not using for loop here for speed!
				while((__R31 & HSYNC) == 0); //wait for HSYNC to go high

				while(__R31 & 0x00010000); //wait for R31[16] to go low 
				while((__R31 & 0x00010000) == 0); //wait for R31[16] to go high
				writeReg = (__R31 & 0x000000fc) << 22; //I am assigning here instead of ORing in order to clear writeReg
				while(__R31 & 0x00010000); 
				while((__R31 & 0x00010000) == 0); 
				writeReg |= (__R31 & 0x000000fc) << 14; 
				while(__R31 & 0x00010000); 
				while((__R31 & 0x00010000) == 0); 
				writeReg |= (__R31 & 0x000000fc) << 6; 
				while(__R31 & 0x00010000); 
				while((__R31 & 0x00010000) == 0); 
				writeReg |= (__R31 & 0x000000fc) >> 2; 

				*addr = writeReg; //write to memory
			}
			curBuf = 1; //switch buffer
			*status &= ~BUF; //clear buf flag for buf 0
		}else{
			//loop through every word in buffer
			for(addr = buf1 ; addr < buf1max ; ++addr)
			{
				//not using for loop here for speed!
				while((__R31 & HSYNC) == 0); //wait for HSYNC to go high

				while(__R31 & 0x00010000); 
				while((__R31 & 0x00010000) == 0); 
				writeReg = (__R31 & 0x000000fc) << 22;
				while(__R31 & 0x00010000); 
				while((__R31 & 0x00010000) == 0); 
				writeReg |= (__R31 & 0x000000fc) << 14; 
				while(__R31 & 0x00010000); 
				while((__R31 & 0x00010000) == 0); 
				writeReg |= (__R31 & 0x000000fc) << 6; 
				while(__R31 & 0x00010000); 
				while((__R31 & 0x00010000) == 0); 
				writeReg |= (__R31 & 0x000000fc) >> 2; 

				*addr = writeReg; //write to memory
			}
			curBuf = 0; //switch buffer
			*status |= BUF; //set buf flag for buf1
		}

		//send finished flag to ARM
		*status |= PRU_2_ARM;
	}
}

//clear all LEDs
void clear()
{
	int i = 0;
	for(; i < 4 ; i++)
		__R30 &= ~LED[i];
}

//flash the specified LED
void flash(char led)
{
	if (led > 3)
		led = 3;
	int i = 0;
	for(; i < 4 ; i++)
	{
		__R30 |= LED[led];
		__delay_cycles(DELAY/50);
		__R30 &= ~LED[led];
		__delay_cycles(DELAY/50);
	}
}

//flash all LEDs
void dance()
{
	flash(0);
	flash(1);
	flash(2);
	flash(3);
}

