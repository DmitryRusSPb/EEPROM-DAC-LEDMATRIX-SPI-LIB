/*
 * audio.c
 *
 *  Created on: 21 дек. 2017 г.
 *      Author: root
 */
#include "audio.h"
#include "defines.h"
#include "STM32_EEPROM_SPI.h"

__IO int16_t outBuffer[2][FRAME_SIZE];
__IO int16_t *outBuff = outBuffer[0];
__IO uint8_t startDecoding;
__IO uint16_t nbFrames;
SpeexBits bits;
// Параметры для функций библиотеки SPEEX
void *encState, *decState;
int quality = 4, complexity=1, vbr=0, enh=1;
int quantityAudio = 0;
char inputBytes[ENCODED_FRAME_SIZE];


void SpeexInit(void)
{
	speex_bits_init(&bits);
	decState = speex_decoder_init(&speex_nb_mode);
	speex_decoder_ctl(decState, SPEEX_SET_ENH, &enh);
}

void PlayMessageFromEEPROM(void)
{
	// Количество блоков по 20 байт
	uint16_t frame_number = 140;
//
//	// Читаем количество фреймов
//	EEPROM_SPI_ReadBuffer((uint8_t*)&frame_number, (uint16_t)EEPROM_READ_ADDR, (uint16_t)1);

	// Счётчик прочитанных байтов
	uint16_t sample_index = 0;

	// Читаем первый блок по 20 байт
	EEPROM_SPI_ReadBuffer((uint8_t*)&inputBytes, (uint16_t)(EEPROM_READ_ADDR + sample_index), (uint16_t)ENCODED_FRAME_SIZE);
	// Увеличиваем счётчик	на количество уже прочитанных байт
	sample_index += 20;

	// Декодируем
	speex_bits_read_from(&bits, inputBytes, ENCODED_FRAME_SIZE);
	speex_decode_int(decState, &bits, (spx_int16_t*)outBuffer[0]);

	// Читаем второй блок по 20 байт
	EEPROM_SPI_ReadBuffer((uint8_t*)&inputBytes, (uint16_t)(EEPROM_READ_ADDR + sample_index), (uint16_t)ENCODED_FRAME_SIZE);
	// Увеличиваем счётчик на количество прочитанных байт
	sample_index += 20;

	// Декодируем
	speex_bits_read_from(&bits, inputBytes, ENCODED_FRAME_SIZE);
	speex_decode_int(decState, &bits, (spx_int16_t*)outBuffer[1]);

	// Счётчик прочитанных блоков
	nbFrames += 2;

	while(nbFrames < frame_number)
	{
		if(startDecoding == 1)
		{
			// Читаем блок по 20 байт
			EEPROM_SPI_ReadBuffer((uint8_t*)&inputBytes, (uint16_t)(EEPROM_READ_ADDR + sample_index), (uint16_t)ENCODED_FRAME_SIZE);
			// Увеличиваем счётчик на количество прочитанных байт
			sample_index += 20;

			// Декодируем
			speex_bits_read_from(&bits, inputBytes, ENCODED_FRAME_SIZE);
			speex_decode_int(decState, &bits, (spx_int16_t*)outBuffer[0]);

			startDecoding = 0;
			nbFrames++;
		}
		if(startDecoding == 2)
		{
			// Читаем блок по 20 байт
			EEPROM_SPI_ReadBuffer((uint8_t*)&inputBytes, (uint16_t)(EEPROM_READ_ADDR + sample_index), (uint16_t)ENCODED_FRAME_SIZE);
			// Увеличиваем счётчик на количество прочитанных байт
			sample_index += 20;

			// Декодируем
			speex_bits_read_from(&bits, inputBytes, ENCODED_FRAME_SIZE);
			speex_decode_int(decState, &bits, (spx_int16_t*)outBuffer[1]);

			startDecoding = 0;
			nbFrames++;
		}

	}//end while

		sample_index = 0;
		nbFrames = 0;
		outBuff = outBuffer[0];
}

void PlayMessage(unsigned char const *array, uint16_t frame_number)
{
	int i;
	uint16_t sample_index = 0;

	for(i=0;i<ENCODED_FRAME_SIZE; i++)
	{
		inputBytes[i] = array[sample_index];
		sample_index++;
	}
	speex_bits_read_from(&bits, inputBytes, ENCODED_FRAME_SIZE);
	speex_decode_int(decState, &bits, (spx_int16_t*)outBuffer[0]);

	for(i=0;i<ENCODED_FRAME_SIZE; i++)
	{
		inputBytes[i] = array[sample_index];
		sample_index++;
	}

	speex_bits_read_from(&bits, inputBytes, ENCODED_FRAME_SIZE);
	speex_decode_int(decState, &bits, (spx_int16_t*)outBuffer[1]);

	nbFrames++;

	while(nbFrames < frame_number)
	{
		if(startDecoding == 1)
		{
			for(i=0;i<ENCODED_FRAME_SIZE; i++)
			{
				inputBytes[i] = array[sample_index];
				sample_index++;
			}

			speex_bits_read_from(&bits, inputBytes, ENCODED_FRAME_SIZE);
			speex_decode_int(decState, &bits, (spx_int16_t*)outBuffer[0]);

			startDecoding = 0;
			nbFrames++;
		}
		if(startDecoding == 2)
		{
			for(i=0;i<ENCODED_FRAME_SIZE; i++)
			{
				inputBytes[i] = array[sample_index];
				sample_index++;
			}

			speex_bits_read_from(&bits, inputBytes, ENCODED_FRAME_SIZE);
			speex_decode_int(decState, &bits, (spx_int16_t*)outBuffer[1]);

			startDecoding = 0;
			nbFrames++;
		}

	}//end while

	sample_index = 0;
	nbFrames = 0;
	outBuff = outBuffer[0];
}

