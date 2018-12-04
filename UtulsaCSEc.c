/*
 * UtulsaCSEc.c
 *
 *  Created on: Aug 27, 2018
 *      Author: Jmaag
 */

#include "CSEc_macros.h"

#define number_of_words(j_local, message_length) ((128*7*(j_local+1) > (message_length + 128))?((((message_length+128)-(128*7*j_local))/32) + 4):(32))

uint8_t flash_error_status = 0;
uint8_t first_iteration = 0;
uint16_t csec_error_bits,page_length;

uint32_t i,j,k;

uint32_t KEY_UPDATE_ENC_C[4] = {0x01015348, 0x45008000, 0x00000000, 0x000000B0};
uint32_t KEY_UPDATE_MAC_C[4] = {0x01025348, 0x45008000, 0x00000000, 0x000000B0};
uint32_t DEBUG_KEY_C[4] = 	   {0x01035348, 0x45008000, 0x00000000, 0x000000B0};
/* Enables CSEc by issuing the Program Partition Command, procedure: Figure 32-8 in RM, Configures for all 20 Keys */

uint8_t configure_part_CSEc(void){

    while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != FTFC_FSTAT_CCIF_MASK); /* Wait until any ongoing flash operation is completed */
    FTFC->FSTAT = (FTFC_FSTAT_FPVIOL_MASK | FTFC_FSTAT_ACCERR_MASK);  /* Write 1 to clear error flags */

    FTFC->FCCOB[3] = 0x80; /* FCCOB0 = 0x80, program partition command */
    FTFC->FCCOB[2] = 0x03; /* FCCOB1 = 2b11, 20 keys */
    FTFC->FCCOB[1] = 0x00; /* FCCOB2 = 0x00, SFE = 0, VERIFY_ONLY attribute functionality disable */
    FTFC->FCCOB[0] = 0x00; /* FCCOB3 = 0x00, FlexRAM will be loaded with valid EEPROM data during reset sequence */
    FTFC->FCCOB[7] = 0x02; /* FCCOB4 = 0x02, 4k EEPROM Data Set Size */
    FTFC->FCCOB[6] = 0x04; /* FCCOB5 = 0x04, no data flash, 64k(all) EEPROM backup */

    FTFC->FSTAT = FTFC_FSTAT_CCIF_MASK; /* Start command execution by writing 1 to clear CCIF bit */

    while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != FTFC_FSTAT_CCIF_MASK); /* Wait until ongoing flash operation is completed */

    flash_error_status = FTFC->FSTAT; /* Read the flash status register for any Execution Error */

    return flash_error_status;
}
/* Get the UID */
uint16_t GET_UID(uint32_t *UID, uint32_t *UID_MAC)
{
	uint32_t challenge_in [4] = {0x12345678, 0x12345678, 0x12345678, 0x12345678};

	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != FTFC_FSTAT_CCIF_MASK); /* Wait until any ongoing flash operation is completed */

	FTFC->FSTAT = (FTFC_FSTAT_FPVIOL_MASK | FTFC_FSTAT_ACCERR_MASK);  /* Write 1 to clear error flags */

	for(i=4,j=0; i<8; i++,j++) // Write to Page1
		CSE_PRAM->RAMn[i].DATA_32= challenge_in[j]; /* Load the Challenge string */

	/* Start command by wring Header */
	CSE_PRAM->RAMn[0].DATA_32= (CMD_GET_ID << 24) | (CMD_FORMAT_COPY << 16) | (CALL_SEQ_FIRST << 8) | (0x00); /*Write to Page0 Word0,  Value = 0x10000000,
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	   No input Key */

	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != FTFC_FSTAT_CCIF_MASK); /* Wait until ongoing CSEc operation is completed */

	csec_error_bits = CSE_PRAM->RAMn[1].DATA_32 >> 16; /*Read Page0 Word1, Error Bits field to check for any CSEc Execution error */

	for(i=8, j=0; i<12; i++, j++) //Read from Page2
		UID[j] = CSE_PRAM->RAMn[i].DATA_32; /* Retrieve the UID */

	for(i=12, j=0; i<16; i++, j++) //Read from Page3
		UID_MAC[j] = CSE_PRAM->RAMn[i].DATA_32; /* Retrieve the UID_MAC, calculated using the MASTER_ECU_KEY */

	return csec_error_bits;
}
/* Initialize Random Number Generator */
uint16_t INIT_RNG(void)
{
	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != FTFC_FSTAT_CCIF_MASK); //Check for the ongoing FLASH command

	FTFC->FSTAT = (FTFC_FSTAT_FPVIOL_MASK | FTFC_FSTAT_ACCERR_MASK);  // Write 1 to clear error flags

	/* Start command by wring Header */
	CSE_PRAM->RAMn[0].DATA_32= (CMD_INIT_RNG << 24) | (CMD_FORMAT_COPY << 16) | (CALL_SEQ_FIRST << 8) | (0x00); //Write to Page0 Word0,  Value = 0x0A000000

	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != 0x80); //Check for the ongoing FLASH command

	csec_error_bits = CSE_PRAM->RAMn[1].DATA_32 >> 16; //Read Page0 Word1, Error Bits

	return csec_error_bits;
}
uint16_t GENERATE_RANDOM_NUMBER(uint32_t *random_number)
{
	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != FTFC_FSTAT_CCIF_MASK); //Check for the ongoing FLASH command

	FTFC->FSTAT = (FTFC_FSTAT_FPVIOL_MASK | FTFC_FSTAT_ACCERR_MASK);  // Write 1 to clear error flags

	/* Start command by wring Header */
	CSE_PRAM->RAMn[0].DATA_32= (CMD_RND << 24) | (CMD_FORMAT_COPY << 16) | (CALL_SEQ_FIRST << 8) | (0x00); //Write to Page0 Word0,  Value = 0x0C000000

	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != 0x80); //Check for the ongoing FLASH command

	csec_error_bits = CSE_PRAM->RAMn[1].DATA_32 >> 16; //Read Page0 Word1, Error Bits

	for(i=4,j=0; i<8; i++,j++) //Read from Page1
		random_number[j] = CSE_PRAM->RAMn[i].DATA_32;

	return csec_error_bits;
}
//unmodified from example
/* Load Secret Keys (Except RAM_KEY) */
uint16_t LOAD_KEY(uint32_t *M4_out, uint32_t *M5_out, uint32_t *M1_in, uint32_t *M2_in, uint32_t *M3_in, uint8_t key_id)
{
	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != FTFC_FSTAT_CCIF_MASK); //Check for the ongoing FLASH command

	FTFC->FSTAT = (FTFC_FSTAT_FPVIOL_MASK | FTFC_FSTAT_ACCERR_MASK);  // Write 1 to clear error flags

	for(i=4,j=0; i<8; i++,j++) //Write to Page1
		CSE_PRAM->RAMn[i].DATA_32 = M1_in[j];

	for(i=8,j=0; i<16; i++,j++) //Write to Page2-3
		CSE_PRAM->RAMn[i].DATA_32 = M2_in[j];

	for(i=16,j=0; i<20; i++,j++) //Write to Page4
		CSE_PRAM->RAMn[i].DATA_32 = M3_in[j];

	/* Start command by wring Header */
	CSE_PRAM->RAMn[0].DATA_32= (CMD_LOAD_KEY << 24) | (CMD_FORMAT_COPY << 16) | (CALL_SEQ_FIRST << 8) | key_id;// Write to Page0 Word0, Value = 0x07000000 | key_id

	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != 0x80); //Check for the ongoing FLASH command

	csec_error_bits = CSE_PRAM->RAMn[1].DATA_32 >> 16; //Read Page0 Word1, Error Bits

	for(i=20,j=0; i<28; i++,j++) //Read from Page5-6
		M4_out[j] = CSE_PRAM->RAMn[i].DATA_32;

	for(i=28,j=0; i<32; i++,j++) //Read from Page7
		M5_out[j] = CSE_PRAM->RAMn[i].DATA_32;

	return csec_error_bits;
}

//unmodified from example
/* Generate the MAC for the data
 * Data copy format feature is used
 * No data size limit
 *  */
uint16_t CMAC(uint32_t *cmac, uint32_t *data, uint8_t key_id, uint32_t message_length)
{

	uint32_t j_local = 0;

	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != FTFC_FSTAT_CCIF_MASK); //Check for the ongoing FLASH command

	FTFC->FSTAT = (FTFC_FSTAT_FPVIOL_MASK | FTFC_FSTAT_ACCERR_MASK);  // Write 1 to clear error flags

	while(j_local <= message_length/(128*7))
	{

		for(i=4; i<32; i++)
			CSE_PRAM->RAMn[i].DATA_32 = data[(i-4) + j_local*28];

		CSE_PRAM->RAMn[3].DATA_32= message_length; // Write to Page0 Word3

		/* Start command by wring Header */
		if(j_local==0)
			CSE_PRAM->RAMn[0].DATA_32=(CMD_GENERATE_MAC << 24) | (CMD_FORMAT_COPY << 16) | (CALL_SEQ_FIRST << 8) | key_id; // Write to Page0 Word0, Value = 0x05000000 | KEY;
		else
			CSE_PRAM->RAMn[0].DATA_32=(CMD_GENERATE_MAC << 24) | (CMD_FORMAT_COPY << 16) | (CALL_SEQ_SUBSEQUENT << 8) | key_id; // Write to Page0 Word0, Value = 0x05000100 | KEY;

	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != 0x80); //Check for the ongoing FLASH command

	csec_error_bits = CSE_PRAM->RAMn[1].DATA_32 >> 16; //Read Page0 Word1, Error Bits
		if(csec_error_bits != 1)
			break;
	j_local++;
	}

	for(i=8; i<12; i++)
		cmac[i-8] = CSE_PRAM->RAMn[i].DATA_32;


	return csec_error_bits;
}

//unmodified from example
/* Generate the MAC for the data
 * Data copy format feature is used
 * No data size limit
 *  */
uint16_t CMAC_VERIFY(uint16_t *verification_status, uint32_t *data_and_cmac, uint8_t key_id, uint32_t message_length)
{
	uint8_t j_local = 0;
	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != FTFC_FSTAT_CCIF_MASK); //Check for the ongoing FLASH command

	FTFC->FSTAT = (FTFC_FSTAT_FPVIOL_MASK | FTFC_FSTAT_ACCERR_MASK);  // Write 1 to clear error flags

	while(j_local <= message_length/(128*7))
	{

		for(i=4; i<32; i++)
			CSE_PRAM->RAMn[i].DATA_32 = data_and_cmac[(i-4) + j_local*28];

		CSE_PRAM->RAMn[3].DATA_32= message_length; // Write to Page0 Word3
		CSE_PRAM->RAMn[2].DATA_32=128 << 16; //Write to MSBs of Page0 Word2

		/* Start command by wring Header */
		if(j_local==0)
			CSE_PRAM->RAMn[0].DATA_32=(CMD_VERIFY_MAC << 24) | (CMD_FORMAT_COPY << 16) | (CALL_SEQ_FIRST << 8) | key_id; // Write to Page0 Word0, Value = 0x05000000 | KEY;
		else
			CSE_PRAM->RAMn[0].DATA_32=(CMD_VERIFY_MAC << 24) | (CMD_FORMAT_COPY << 16) | (CALL_SEQ_SUBSEQUENT << 8) | key_id; // Write to Page0 Word0, Value = 0x05000100 | KEY;

	while((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) != 0x80); //Check for the ongoing FLASH command

	csec_error_bits = CSE_PRAM->RAMn[1].DATA_32 >> 16; //Read Page0 Word1, Error Bits
	j_local++;
	}

	*verification_status = CSE_PRAM->RAMn[5].DATA_32 >> 16; //Read Page1 Word1, Error Bits

	return csec_error_bits;
}
