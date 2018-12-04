/*
 * UtulsaFTM_Counter.c
 *
 *  Created on: Sep 18, 2018
 *      Author: Jmaag
 */

#include "S32K144.h" /* include peripheral declarations S32K144 */

void init_free_counter(void){
	  PCC->PCCn[PCC_FTM0_INDEX] &= ~PCC_PCCn_CGC_MASK; 	/* Ensure clk disabled for config */
	  PCC->PCCn[PCC_FTM0_INDEX] |= PCC_PCCn_PCS(0b001)	/* Clock Src=1, 8 MHz SOSCDIV1_CLK */
	                                |  PCC_PCCn_CGC_MASK;   /* Enable clock for FTM regs */
	  FTM0[0].CNTIN = 0; /*initize counter with 0*/
	  FTM0[0].MODE = FTM_MODE_WPDIS(1); /*write protection disabled */
	  FTM0[0].SC = 13; /* clock source selection: System clock*/
	  	  	  	  	   /*prescale divider is 5 (/32 I think)*/
	  FTM0[0].MOD = 0xFFFF; /*Counter goes to 0xFFFF*/
	  FTM0[0].MODE = FTM_MODE_FTMEN(0); /* FTM enable*/
}

uint16_t get_count(void){
	uint16_t count = FTM0->CNT;
	return count;
}
