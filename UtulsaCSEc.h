/*
 * UtulsaCSEc.h
 *
 *  Created on: Aug 27, 2018
 *      Author: Jmaag
 */

#ifndef UTULSACSEC_H_
#define UTULSACSEC_H_

uint8_t configure_part_CSEc(void);
uint16_t GET_UID(uint32_t *UID, uint32_t *UID_MAC);
uint16_t INIT_RNG(void);
uint16_t GENERATE_RANDOM_NUMBER(uint32_t *random_number);
//uint16_t LOAD_RAM_KEY(uint32_t *key);
//uint16_t EXPORT_RAM_KEY(uint32_t *M1_out, uint32_t *M2_out, uint32_t *M3_out, uint32_t *M4_out, uint32_t *M5_out);
uint16_t LOAD_KEY(uint32_t *M4_out, uint32_t *M5_out, uint32_t *M1_in, uint32_t *M2_in, uint32_t *M3_in, uint8_t key_id);
uint16_t CMAC(uint32_t *cmac, uint32_t *data, uint8_t key_id, uint32_t message_length);
uint16_t CMAC_VERIFY(uint16_t *verification_status, uint32_t *data_and_cmac, uint8_t key_id, uint32_t message_length);

#endif /* UTULSACSEC_H_ */
