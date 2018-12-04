/*
 * UtulsaFlexCAN.h
 *
 *  Created on: Aug 12, 2018
 *      Author: Jmaag
 */

#ifndef UTULSAFLEXCAN_H_
#define UTULSAFLEXCAN_H_

struct CAN_data {
	uint32_t  code;              /* Received message buffer code */
	uint32_t  extID;             /* Received message ID */
	uint32_t  dataLen;           /* Recieved message number of data bytes */
	uint32_t  data[2];           /* Received message data (2 words) */
	uint32_t  timestamp;         /* Received message time */
	//initilize() {code = extID = dataLen = data[0] = data[1] = timestamp = 0; }
};

struct wheel_speed {
	uint32_t LHFront;
	uint32_t RHFront;
	uint32_t LHRear;
	uint32_t RHRear;
};

//Initilizes the right clock pins and can pins on board
void PORT_CAN_init (void);
//Turns on CAN for 250k bus extended ID's
void FLEXCAN0_init_250k_extIDs (void);
//Transmits a message of CAN_data type
void FLEXCAN0_tx (struct CAN_data);
//Returns CAN_data recieved
struct CAN_data FLEXCAN0_rx (void);
//Checks for an incoming message 1 = message availble, 0 = no message
int FLEXCAN0_FIFO_available(void);
//takes pwm input (range ~= 600->64000 and maps 4 speeds closely to 0-170 kph
struct wheel_speed Wheel_speeds_from_pots(int, int, int);
//takes #cmacs and a clock count input and frames into a synchronization messsage
struct CAN_data Msg_framed_with_counts(uint64_t, uint32_t);
//takes wheel speed data and frames 0xFE6E*
struct CAN_data Msg_framed_with_wheel_speeds_high_resolution(struct wheel_speed);
//takes wheel speed data and frames 0xFEBF*
struct CAN_data Msg_framed_with_wheel_speeds_front_axle(struct wheel_speed);
//takes the cmac data and frams on top of wheel speed 0xFE6E*
struct CAN_data Msg_framed_with_cmac(struct CAN_data,uint32_t *cmac,int *cmac_location);

#endif /* UTULSAFLEXCAN_H_ */


