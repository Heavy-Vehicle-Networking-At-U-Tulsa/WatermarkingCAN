/*
 * main implementation: use this 'C' sample to create your own application
 *
 */
//still need randomness for speed GOOD FOR NOW
//***still need to add cmac
//still need to fine tune timing? idk how GOOD FOR NOW

#include "S32K144.h" /* include peripheral declarations S32K144 */
#include "UtulsaFlexCAN.h"
#include "clocks_and_modes.h"
#include "ADC.h"
#include "UtulsaLPIT.h"
#include "UtulsaCSEc.h"
#include "UtulsaFTM_Counter.h"
//#include "CSEc_functions.h"
#include "CSEc_Macros.h"
#include "LPUART.h"
#include <stdio.h>

#define PTC14 14        /* Port PTC12, bit 12: FRDM EVB input from BTN0 [SW2] */


void WDOG_disable (void){
  WDOG->CNT=0xD928C520; 	/* Unlock watchdog */
  WDOG->TOVAL=0x0000FFFF;	/* Maximum timeout value */
  WDOG->CS = 0x00002100;    /* Disable watchdog */
}

void PORT_init (void) {
  PCC->PCCn[PCC_PORTC_INDEX ]|=PCC_PCCn_CGC_MASK; /* Enable clock for PORTC */
  PORTC->PCR[6]|=PORT_PCR_MUX(2);           /* Port C6: MUX = ALT2,UART1 TX */
  PORTC->PCR[7]|=PORT_PCR_MUX(2);           /* Port C7: MUX = ALT2,UART1 RX */
}

void send_can_uart(struct CAN_data message){
	 char test_str[9] = {0,0,0,0,0,0,0,0,0};
	 snprintf(test_str,9,"%08X",message.extID);
	 LPUART1_transmit_string(test_str);
	 snprintf(test_str,9,"%08X",message.data[0]);
	 LPUART1_transmit_string(test_str);
	 snprintf(test_str,9,"%08X",message.data[1]);
	 LPUART1_transmit_string(test_str);
	 //LPUART1_transmit_char(' ');
	 LPUART1_transmit_char('\r');
	 LPUART1_transmit_char('\n');

}


/*can data framing*/
struct CAN_data txmessage;
struct wheel_speed msgData;
/*output of csec functions -> 1 means no error*/
uint32_t csec_error;
/*wheel speed inputs*/
uint32_t main_pot = 0;
uint32_t fade_pot = 0;
uint32_t balance_pot = 0;
/*messages sent since last synchronization*/
static volatile uint32_t messageCount = 0;
/*interrupts occured since last synchronization*/
static volatile uint32_t interruptCount = 0;
/*data that is cmac'd*/
uint32_t can_data[24] = {
		0x0123,0x4567,0x1234, 0x5678,0x2345, 0x6789,0x0123, 0x4567,0x1234, 0x5678,0x2345,0x6789,0x0123,0x4567,0x1234,0x5678,0x2345,0x6789,0x0123,0x4567,0x1234,0x5678
};
uint32_t cmac[4];
int cmac_location = 0;
/*clock count and iteration count variables*/
static uint16_t clock_count = 0;
static uint64_t total_count = 0;
static uint32_t msb_clock_count = 0;
static uint16_t previous_clock_count = 0;
static uint32_t iteration_count = 0;
//UART character stream
char test_str[9] = {0,0,0,0,0,0,0,0,0};



int main(void)
{

	  //we gon have to do a lot of work here

	  WDOG_disable();
	  SOSC_init_8MHz();       /* Initialize system oscillator for 8 MHz xtal */
	  SPLL_init_160MHz();     /* Initialize SPLL to 160 MHz with 8 MHz SOSC */
	  NormalRUNmode_80MHz();  /* Init clocks: 80 MHz sysclk & core, 40 MHz bus, 20 MHz flash */

	  FLEXCAN0_init_250k_extIDs();         /* Init FlexCAN0 */
	  PORT_CAN_init();             /* Configure CAN ports */
	  ADC_init();            /* Init ADC resolution 12 bit*/
	  PCC-> PCCn[PCC_PORTD_INDEX] = PCC_PCCn_CGC_MASK; /* Enable clock for PORT D */
	  NVIC_init_IRQs();        /* Enable desired interrupts and priorities */
	  LPIT0_init();            /* Initialize PIT0_ch0, ch1, and ch2  */
	  init_free_counter();     /*Initialize FTM to free counting up mode*/
	  PORT_init();           /* Configure ports */
	  LPUART1_init();        /* Initialize LPUART @ 115200*/ //need to change this to higher at somepoint
	  PCC-> PCCn[PCC_PORTC_INDEX] = PCC_PCCn_CGC_MASK; /* Enable clock to PORT C */
	  PTC->PDDR &= ~(1<<PTC14);    /* Port C14: Data Direction= input (default) */
	  PORTC->PCR[14] = 0x00000110; /* Port C14: MUX = GPIO, input filter enabled */
	                               /* Configure port D0 as GPIO output (LED on EVB) */

	  csec_error = INIT_RNG(); //Randum number generator must be initialize before generating random number
	  csec_error = CMAC(cmac, can_data, KEY_11, 128);
	  //printf("here is CMAC: %X %X %X %X\n",cmac[0],cmac[1],cmac[2],cmac[3]);
	  LPUART1_transmit_char('*');
	  snprintf(test_str,9,"%X",cmac[0]);
	  LPUART1_transmit_string(test_str);
	  snprintf(test_str,9,"%X",cmac[1]);
	  LPUART1_transmit_string(test_str);
	  snprintf(test_str,9,"%X",cmac[2]);
	  LPUART1_transmit_string(test_str);
	  snprintf(test_str,9,"%X",cmac[3]);
	  LPUART1_transmit_string(test_str);



        for(;;) {

            convertAdcChan(9);                   /* Convert Channel pin c1*/
            while((adc_complete()==0)){}            /* Wait for conversion complete flag */
            main_pot = read_adc_chx();       /* Get channel's conversion results in mv */

            convertAdcChan(10);                   /* Convert Channel pin c2 */
            while((adc_complete()==0)){}            /* Wait for conversion complete flag */
            balance_pot = read_adc_chx();       /* Get channel's conversion results in mv */

            convertAdcChan(13);                   /* Convert Channel pin c5*/
            while((adc_complete()==0)){}            /* Wait for conversion complete flag */
            int temp;
            temp = (read_adc_chx());       /* Get channel's conversion results in mv */
            fade_pot = temp;



        }



    /* to avoid the warning message for GHS and IAR: statement is unreachable*/
#if defined (__ghs__)
#pragma ghs nowarning 111
#endif
#if defined (__ICCARM__)
#pragma diag_suppress=Pe111
#endif
	return 0;
}


//this is sending every 20ms
       void LPIT0_Ch0_IRQHandler (void) {

        	LPIT0->MSR |= LPIT_MSR_TIF0_MASK; /* Clear LPIT0 timer flag 0 */
        	msgData = Wheel_speeds_from_pots(main_pot,fade_pot,balance_pot);
        	txmessage = Msg_framed_with_wheel_speeds_high_resolution(msgData);

          	messageCount++;
        	txmessage.data[0] = txmessage.data[0] + ((messageCount) << (56-32));
        	can_data[(messageCount-1)*2] = txmessage.data[0];
            can_data[((messageCount-1)*2)+1 ] = txmessage.data[1];
            txmessage = Msg_framed_with_cmac(txmessage,cmac,&cmac_location);
        	FLEXCAN0_tx(txmessage);     /* Transmit message using MB6 */
        	LPUART1_transmit_char('%');
        	send_can_uart(txmessage);
        	interruptCount++;

        	 if ((PTC->PDIR & (1<<PTC14)) == 0) {   /* If Pad Data Input = 1 (BTN0 [SW2] pushed) */
        	        txmessage.data[0] = 0xA2FAFF5A;
        	        txmessage.data[1] = 0xDFFFFFFF;
        	        txmessage.extID = 0xC00000B;
        	        FLEXCAN0_tx(txmessage);     /* Transmit message using MB6 */
        	        LPUART1_transmit_char('!');
        	        send_can_uart(txmessage);
        	        txmessage.data[0] = 0x006E006E;
        	        txmessage.data[1] = 0x006E006E;
        	        txmessage.extID = 0x18FE6E0B;
        	        FLEXCAN0_tx(txmessage);
        	    	LPUART1_transmit_char('%');
        	        send_can_uart(txmessage);
        	 }


        	//every 100ms
        	if(interruptCount == 6){

        	      txmessage = Msg_framed_with_wheel_speeds_front_axle(msgData);
        	      //add_to_can_data();

        	      messageCount++;
        	      txmessage.data[0] = txmessage.data[0] + ((messageCount) << (56-32));
        	      can_data[(messageCount-1)*2] = txmessage.data[0];
        	      can_data[((messageCount-1)*2)+1 ] = txmessage.data[1];
        	      FLEXCAN0_tx(txmessage);     /* Transmit message using MB6 */
        	      LPUART1_transmit_char('^');
        	      send_can_uart(txmessage);
        	}


        	//every 180ms
        	if(interruptCount == 10){
        		  /*absolute counter calculation*/
        		  clock_count = get_count();
        		  if(clock_count < previous_clock_count){
        			  msb_clock_count++;
        		  }
        		  total_count = (msb_clock_count<<16)+clock_count;
        	      previous_clock_count = clock_count;
        	      /*end*/
        	      /*CAN synchronization generation*/
        		  txmessage = Msg_framed_with_counts(total_count, iteration_count);
        		  can_data[messageCount*2] = txmessage.data[0];
        		  can_data[(messageCount*2)+1 ] = txmessage.data[1];
        		  messageCount++;
        		  //add_to_can_data();
        		  FLEXCAN0_tx(txmessage);
        		  LPUART1_transmit_char('@');
        		  send_can_uart(txmessage);
        		  /*end*/


        		  /*New cmac generation*/
        		  uint32_t can_data_for_cmac[2*messageCount];
        		  int i;
        		  for(i=0;i<(2*messageCount);i++){
        			  can_data_for_cmac[i] = can_data[i];
        		  }
        		  csec_error = CMAC(cmac, can_data_for_cmac, KEY_11, (24*32));


        		  //printf("here is CMAC: %X %X %X %X\n",cmac[3],cmac[2],cmac[1],cmac[0]);
        		  LPUART1_transmit_char('*');
        		  snprintf(test_str,9,"%08X",cmac[3]);
        		  LPUART1_transmit_string(test_str);
        		  snprintf(test_str,9,"%08X",cmac[2]);
        		  LPUART1_transmit_string(test_str);
        		  snprintf(test_str,9,"%08X",cmac[1]);
        		  LPUART1_transmit_string(test_str);
        		  snprintf(test_str,9,"%08X",cmac[0]);
        		  LPUART1_transmit_string(test_str);
        		  LPUART1_transmit_char('\r');
        		  LPUART1_transmit_char('\n');
        		  messageCount = 0;
        		  /*end*/
        		 //
        		  cmac_location = 0;
        		  interruptCount = 0;
        		  iteration_count++;
        	}

        }


