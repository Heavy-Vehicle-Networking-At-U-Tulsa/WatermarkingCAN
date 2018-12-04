/*
 * UtulsaFlexCAN.c
 *
 *  Created on: Aug 12, 2018
 *      Author: Jmaag
 */

#include "S32K144.h" /* include peripheral declarations S32K144 */
#include "UtulsaFlexCAN.h"
#include "UtulsaCSEc.h"

uint32_t  RxCODE;              /* Received message buffer code */
uint32_t  RxID;                /* Received message ID */
uint32_t  RxLENGTH;            /* Recieved message number of data bytes */
uint32_t  RxDATA[2];           /* Received message data (2 words) */
uint32_t  RxTIMESTAMP;         /* Received message time */
//volatile uint32_t messageCount = 0;

void PORT_CAN_init (void) {
  PCC->PCCn[PCC_PORTE_INDEX] |= PCC_PCCn_CGC_MASK; /* Enable clock for PORTE */
  PORTE->PCR[4] |= PORT_PCR_MUX(5); /* Port E4: MUX = ALT5, CAN0_RX */
  PORTE->PCR[5] |= PORT_PCR_MUX(5); /* Port E5: MUX = ALT5, CAN0_TX */
  PCC->PCCn[PCC_PORTD_INDEX ]|=PCC_PCCn_CGC_MASK;   /* Enable clock for PORTD */
}

void FLEXCAN0_init_250k_extIDs(void) {
#define MSG_BUF_SIZE  4    /* Msg Buffer Size. (CAN 2.0AB: 2 hdr +  2 data= 4 words) */
  uint32_t   i=0;

  PCC->PCCn[PCC_FlexCAN0_INDEX] |= PCC_PCCn_CGC_MASK; /* CGC=1: enable clock to FlexCAN0 */
  CAN0->MCR |= CAN_MCR_MDIS_MASK;         /* MDIS=1: Disable module before selecting clock */
  CAN0->CTRL1 &= ~CAN_CTRL1_CLKSRC_MASK;  /* CLKSRC=0: Clock Source = oscillator (8 MHz) */
  CAN0->MCR &= ~CAN_MCR_MDIS_MASK;        /* MDIS=0; Enable module config. (Sets FRZ, HALT)*/
  while (!((CAN0->MCR & CAN_MCR_FRZACK_MASK) >> CAN_MCR_FRZACK_SHIFT))  {}
                 /* Good practice: wait for FRZACK=1 on freeze mode entry/exit */
  CAN0->CTRL1 = 0x01DB0006; /* Configure for 250 KHz bit time */
                            /* Time quanta freq = 16 time quanta x 250 KHz bit time= 4MHz */
                            /* PRESDIV+1 = Fclksrc/Ftq = 8 MHz/4 MHz = 2 */
                            /*    so PRESDIV = 1 */
                            /* PSEG2 = Phase_Seg2 - 1 = 4 - 1 = 3 */
                            /* PSEG1 = PSEG2 = 3 */
                            /* PROPSEG= Prop_Seg - 1 = 7 - 1 = 6 */
                            /* RJW: since Phase_Seg2 >=4, RJW+1=4 so RJW=3. */
                            /* SMP = 1: use 3 bits per CAN sample */
                            /* CLKSRC=0 (unchanged): Fcanclk= Fosc= 8 MHz */
  for(i=0; i<128; i++ ) {   /* CAN0: clear 32 msg bufs x 4 words/msg buf = 128 words*/
    CAN0->RAMn[i] = 0;      /* Clear msg buf word */
  }
  for(i=0; i<16; i++ ) {          /* In FRZ mode, init CAN0 16 msg buf filters */
    CAN0->RXIMR[i] = 0xFFFFFFFF;  /* Check all ID bits for incoming messages */
  }
  CAN0->RAMn[ 5*MSG_BUF_SIZE] = 0x7FFFFFFF;
  CAN0->RAMn[ 6*MSG_BUF_SIZE] = 0x7FFFFFFF;
  CAN0->RXFGMASK = 0x000000000;
  //CAN0->RXMGMASK = 0x1FFFFFFF;/* Global acceptance mask: check all ID bits */
  //CAN0->RAMn[ 4*MSG_BUF_SIZE + 0] = 0x04600000; /* Msg Buf 4, word 0: Enable for reception */
                                                /* EDL,BRS,ESI=0: CANFD not used */
                                                /* CODE=4: MB set to RX inactive */
                                                /* IDE=1: Ext ID */
  	  	  	  	  	  	  	  	  	  	  	  	/* SRR = 1: needed for ext id*/
                                                /* RTR, TIME STAMP = 0: not applicable */
  	  	  	  	  	  	  	  	  	  	  	  	/*Don't Define Addresses Here*/
                                                /* PRIO = 0: CANFD not used */
  CAN0->MCR = 0x2000001F;       /* Negate FlexCAN 1 halt state for 32 MBs */
  	  	  	  	  	  	  	  	/* Set RXFIFO bit 29 = 1 */
  while ((CAN0->MCR && CAN_MCR_FRZACK_MASK) >> CAN_MCR_FRZACK_SHIFT)  {}
                 /* Good practice: wait for FRZACK to clear (not in freeze mode) */
  while ((CAN0->MCR && CAN_MCR_NOTRDY_MASK) >> CAN_MCR_NOTRDY_SHIFT)  {}
                 /* Good practice: wait for NOTRDY to clear (module ready)  */
}

int FLEXCAN0_FIFO_available(void){
	return (CAN0->IFLAG1 >> 5) & 1;
}


void FLEXCAN0_tx(struct CAN_data message) { /* Assumption:  Message buffer CODE is INACTIVE */
  CAN0->IFLAG1 = 0x00000100;       /* Clear CAN 0 MB 8 flag without clearing others*/
  //if((message.data[0] >> 24) == 0x11){
	//  messageCount = 0;
 // }
  //message.data[0] = message.data[0] + (messageCount << (56-32));
  //messageCount++;
  CAN0->RAMn[ 8*MSG_BUF_SIZE + 2] = message.data[0]; /* MB0 word 2: data word 0 */
  CAN0->RAMn[ 8*MSG_BUF_SIZE + 3] = message.data[1]; /* MB0 word 3: data word 1 */
  CAN0->RAMn[ 8*MSG_BUF_SIZE + 1] = message.extID; /* MB0 word 1: Tx msg ID */
  CAN0->RAMn[ 8*MSG_BUF_SIZE + 0] = 0x0C600000 | message.dataLen <<CAN_WMBn_CS_DLC_SHIFT; /* MB0 word 0: */
                                                /* EDL,BRS,ESI=0: CANFD not used */
                                                /* CODE=0xC: Activate msg buf to transmit */
                                                /* IDE=0: Standard ID */
                                                /* SRR=1 Tx frame (not req'd for std ID) */
                                                /* RTR = 0: data, not remote tx request frame*/
                                                /* DLC = input */
}

//done now :)
struct CAN_data FLEXCAN0_rx(void) {  /* Receive msg from ID 0x556 using msg buffer 4 */
  uint8_t j;
  //uint32_t dummy;
  struct CAN_data data;

  data.code   = (CAN0->RAMn[ 0*MSG_BUF_SIZE + 0] & 0x07000000) >> 24;  /* Read CODE field */
  data.extID    = (CAN0->RAMn[ 0*MSG_BUF_SIZE + 1] & CAN_WMBn_ID_ID_MASK)  >> CAN_WMBn_ID_ID_SHIFT ;
  data.dataLen = (CAN0->RAMn[ 0*MSG_BUF_SIZE + 0] & CAN_WMBn_CS_DLC_MASK) >> CAN_WMBn_CS_DLC_SHIFT;
  for (j=0; j<2; j++) {  /* Read two words of data (8 bytes) */
    data.data[j] = CAN0->RAMn[ 0*MSG_BUF_SIZE + 2 + j];
  }
  if (data.dataLen <=4){
	  data.data[1] = 0x0000;
	  data.data[0] = (data.data[0] >> 2*(4-data.dataLen)) << (2*(4-data.dataLen));
  }
  else{
	  data.data[1] =(data.data[1] >> 2*(8-(data.dataLen))) << (2*(8-data.dataLen));
  }
  data.timestamp = (CAN0->RAMn[ 0*MSG_BUF_SIZE + 0] & 0x000FFFF);
  //dummy = CAN0->TIMER;             /* Read TIMER to unlock message buffers */
  CAN0->IFLAG1 = 0x00000020;       /* Clear CAN 0 MB 1 flag without clearing others*/
  return data;
}

struct wheel_speed Wheel_speeds_from_pots(int main_pot, int fade_pot, int bal_pot){
	struct wheel_speed data;
	uint32_t kph;
	int fade;
	int bal;
	uint8_t lbal;
	uint8_t rbal;
	uint8_t ffade;
	uint8_t bfade;
	//map range 0-5000 to ~0-110kph
	kph = ((main_pot)*110)/5000;
	//map range 0-5000 to -3 - 3
	fade = (((fade_pot)*7)/5000)-3;
	//map range 0-5000 to -3 - 3
    bal = (((bal_pot)*7)/5000)-3;

    if(fade == 0){
    	ffade = 0;
    	bfade = 0;

    }
    else if(fade < 0){
    	bfade = abs(fade);
    	ffade = 0;
    	kph = kph +1;
    }
    else{
    	ffade = fade;
    	bfade = 0;
    	if(kph != 0){
    		kph = kph -1;
    	}
    }
    if(bal == 0){
    	rbal = 0;
    	lbal = 0;
    }
    else if(bal <0){
    	lbal = abs(bal);
    	rbal = 0;
    }
    else{
    	rbal = bal;
    	lbal = 0;
    }

	if(kph == 0){
		data.LHFront = 0;
		data.RHFront = 0;
		data.LHRear = 0;
		data.RHRear = 0;
	}
	else{
		data.LHFront = (kph<<2) + lbal + ffade;
	    data.RHFront = (kph<<2) + rbal + ffade;
	    data.LHRear = (kph<<2) + lbal + bfade;
	    data.RHRear = (kph<<2) + rbal + bfade;
	}

	return data;
}

struct CAN_data Msg_framed_with_wheel_speeds_high_resolution(struct wheel_speed speeds){
	struct CAN_data data;
	data.data[1]= 0;
	data.data[2] = 0;

	//need to check standard for accuracy here where each wheel goes...
	uint8_t lsb = (speeds.LHFront & 0x00003) << 6;
	uint8_t msb = speeds.LHFront >> 2;
	data.data[1] = (msb<<16) + (lsb<<24);
	lsb = (speeds.RHFront & 0x00003) << 6;
	msb = speeds.RHFront >> 2;
	data.data[1] = data.data[1] + msb + (lsb<<8);
	lsb = (speeds.LHRear & 0x00003) << 6;
	msb = speeds.LHRear >> 2;
	data.data[0] = (msb<<16) + (lsb<<24);
	lsb = (speeds.RHRear & 0x00003) << 6;
	msb = speeds.RHRear >> 2;
	data.data[0] = data.data[0] + msb + (lsb<<8);
	data.dataLen = 8;
	data.extID = 0x18FE6E0B;
	return data;
}

struct CAN_data Msg_framed_with_wheel_speeds_front_axle(struct wheel_speed speeds){
	struct CAN_data data;
	data.data[1]= 0;
	data.data[0] = 0;
	uint32_t front_axle = (speeds.LHFront + speeds.RHFront)/2;
	uint32_t rear_axle = (speeds.LHRear + speeds.RHRear)/2;
	//need to check standard for accuracy here where each wheel goes...
	uint32_t lsb = (front_axle & 0x00003) << 6;
	uint32_t msb = front_axle >> 2;
	data.data[1] = (msb<<16) + (lsb<<24);
	//3
	uint32_t left_to_front = ((speeds.LHFront - front_axle) + 7.18125)*16;
	//4
	uint32_t right_to_front = ((speeds.RHFront - front_axle) + 7.18125)*16;
	//5&7
	uint32_t left_to_rear = ((speeds.LHRear - rear_axle) + 7.18125)*16;
	//6&8
	uint32_t right_to_rear = ((speeds.RHRear - rear_axle) + 7.18125)*16;

	data.data[0] = data.data[1] + right_to_front + (left_to_front<<8);
	data.data[1] = (right_to_rear<<16) + (left_to_rear<<24) + right_to_rear + (left_to_rear<<8);
	data.dataLen = 8;
	data.extID = 0x18FEBF0B;
	return data;
}

struct CAN_data Msg_framed_with_counts(uint64_t total_count, uint32_t iteration_count){
	struct CAN_data data;
	data.data[0] = (iteration_count<<8)+((total_count<<24)>>32);
	data.data[1] = (total_count<<32)>>32;
	data.dataLen = 8;
	data.extID = 0x1801000B;
	return data;
}

struct CAN_data Msg_framed_with_cmac(struct CAN_data txmessage ,uint32_t *cmac,int *cmac_location){

	int limit = 18;
	int cmac_to_add_to_message = 0;
	if (((*cmac_location) + 18) >=128){
		limit = 128 - (*cmac_location);

	}
	int test = cmac[1];
	int i = *cmac_location;
	for(i = 0;i<limit;i++){
		cmac_to_add_to_message = cmac_to_add_to_message + (((cmac[(*cmac_location)/32]>>((*cmac_location)%32)) & 0x1) << (i));
		(*cmac_location) = (*cmac_location)+1;
	}

	txmessage.data[0] = txmessage.data[0] + ((((cmac_to_add_to_message << 26) >>26)&0x3F) <<8);
	txmessage.data[1] = txmessage.data[1] + ((((cmac_to_add_to_message << 20) >> 26 )&0x3F)<<24) + ((((cmac_to_add_to_message << 14 ) >> 26)&0x3F)<<8);
	return txmessage;
}
