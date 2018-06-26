/*
 * max7219.c
 *
 *  Created on: 21 дек. 2017 г.
 *      Author: root
 */
#include "max7219.h"

extern SPI_HandleTypeDef hspi3;
extern uint8_t playSound;
extern osSemaphoreId chipSelectSemHandle;

uint16_t max7219DigitRegisters[8] =
{0x01 << 8, 0x02 << 8, 0x03 << 8, 0x04 << 8, 0x05 << 8, 0x06 << 8, 0x07 << 8, 0x08 << 8};

void sendData(uint16_t data)
{
	// Забираем семафор, чтобы другие задачи не смогли опустить CS
	osSemaphoreWait(chipSelectSemHandle, osWaitForever);
	// reset chipselect line, in oder to chose MAX729
	HAL_GPIO_WritePin(PORT_NCC, PIN_NCC, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi3, (uint8_t*)&data, 1, 500);
	// set chipselect line
	HAL_GPIO_WritePin(PORT_NCC, PIN_NCC, GPIO_PIN_SET);
	// Отдаём семафор, чтобы другие задачи смогли опустить CS
	osSemaphoreRelease(chipSelectSemHandle);
}

void MAX729_SetIntensivity(uint8_t intensivity)
{
	if (intensivity > 0x0F) return;
	// test of indicators is off
	sendData(REG_DISPLAY_TEST | 0x00);
	// wake up
	sendData(REG_SHUTDOWN | 0x01);
	// quantity of register = number of row of our matrix = 7(0,1,2,3,4,5,6,7)
	sendData(REG_SCAN_LIMIT | 0x07);
	// intensivity of brightness (from 0...0xFF)
	sendData(REG_INTENSITY | intensivity);
	// decoders are turned off
	sendData(REG_DECODE_MODE | 0x00);
}

void MAX729_Clean(void)
{
	// Чистка LED матрицы
	sendData(REG_DIGIT_0 | 0x00);
	sendData(REG_DIGIT_1 | 0x00);
	sendData(REG_DIGIT_2 | 0x00);
	sendData(REG_DIGIT_3 | 0x00);
	sendData(REG_DIGIT_4 | 0x00);
	sendData(REG_DIGIT_5 | 0x00);
	sendData(REG_DIGIT_6 | 0x00);
	sendData(REG_DIGIT_7 | 0x00);
}

void MAX729_Init(uint8_t intensivity)
{
	MAX729_SetIntensivity(intensivity);
	MAX729_Clean();
}

// Двигаем изображение по горизонтали
EXIT DrawGor(uint8_t* array, int symbol, uint8_t speed)
{
	// Переменная для хранения...(скроллинг)
	uint8_t q = 0;

	// Здесь описан момент движения (справа налево) изоображения до
	// тех пор, пока оно не будет полностью видно
	for(uint8_t i = 8; i > 0; i--)
	{
		for(uint8_t j = 0; j < 8; j++)
		{

			// Делаем битовый сдвиг (двигаем изображение)
			q = array[8*symbol + j] << i;
			// Отправляем данные LedMatrix
			sendData(max7219DigitRegisters[j] | q);
		}
		if(playSound) return TIME_TO_GO;
		osDelay(SPEED*speed);
	}
	// Здесь описан момент движения (справа налево) изоображения с положения
	// нормальной видимости до его полного исчезновения
	for(uint8_t i = 0; i < 8; i++)
	{
		for(uint8_t j = 0; j < 8; j++)
		{

			// Делаем битовый сдвиг (двигаем изображение)
			q = array[8*symbol + j] >> i;
			// Отправляем данные LedMatrix
			sendData(max7219DigitRegisters[j] | q);
		}
		if(playSound) return TIME_TO_GO;
		osDelay(SPEED*speed);
	}
	return NOT_TIME_TO_GO;
}

EXIT DrawVert(uint8_t* array, int symbol, uint8_t speed) // Зажигает светодиоиды в LED matrix
{
	// Переменная для хранения...(скроллинг)
	uint8_t q = 0;

	// Здесь описан момент "поднятия" изоображения снизу до
	// тех пор, пока оно не будет полностью видно
	for(uint8_t i = 8; i > 0; i--)
	{
		for(uint8_t j = 0; j < 8; j++)
		{

			// Если сумма номера столбца и
			// количества сдвигов больше 7, то это значит,
			// что изображение ещё не дошло до данной строчки
			// и, следовательно, нужно показать пустоту.
			if((j+i) > 7)
			{
				q = 0;
			}
			else
			{
				q = array[8*symbol + j + i];
			}
			// Отправляем данные LedMatrix
			sendData(max7219DigitRegisters[j] | q);
		}
		if(playSound) return TIME_TO_GO;
		osDelay(SPEED*speed);
	}

	// Здесь описан момент "поднятия" изоображения с положения
	// нормальной видимости до его полного исчезновения
	for(uint8_t i = 0; i <= 8; i++)
	{
		for(uint8_t j = 0; j < 8; j++)
		{

			// Если разница между номером столбца и
			// количеством сдвигов меньше нуля, то это значит,
			// что изображение уже прошло данную строку
			// и, следовательно, нужно показать пустоту.
			if((j-i) < 0)
			{
				q = 0;
			}
			else
			{
				q = array[8*symbol + j - i];
			}
			// Отправляем данные LedMatrix
			sendData(max7219DigitRegisters[j] | q);
		}
		if(playSound) return TIME_TO_GO;
		osDelay(SPEED*speed);
	}
	return NOT_TIME_TO_GO;
}

EXIT DrawTS(uint8_t* array, int symbol, uint8_t speed)
{
	// Переменная для хранения...(скроллинг)
	uint8_t q = 0;

	// Здесь описан момент движения (справа налево) изоображения с положения
	// нормальной видимости до его полного исчезновения
	Draw(array, symbol);
	for(uint8_t j = 0; j < 8; j++)
	{
		// Делаем битовый сдвиг (двигаем изображение)
		q = array[8*symbol + j] >> 1;
		// Отправляем данные LedMatrix
		sendData(max7219DigitRegisters[j] | q);
	}
	if(playSound) return TIME_TO_GO;
	osDelay(SPEED*speed);

	for(uint8_t j = 0; j < 8; j++)
	{
		// Делаем битовый сдвиг (двигаем изображение)
		q = array[8*symbol + j - 1] >> 1;
		// Отправляем данные LedMatrix
		sendData(max7219DigitRegisters[j] | q);
		sendData(max7219DigitRegisters[0] | 0);
	}
	if(playSound) return TIME_TO_GO;
	osDelay(SPEED*speed);

	for(uint8_t j = 0; j < 8; j++)
	{
		// Делаем битовый сдвиг (двигаем изображение)
		q = array[8*symbol + j - 1];
		// Отправляем данные LedMatrix
		sendData(max7219DigitRegisters[j] | q);
		sendData(max7219DigitRegisters[0] | 0);
	}
	if(playSound) return TIME_TO_GO;
	osDelay(SPEED*speed);

	for(uint8_t j = 0; j < 8; j++)
	{
		// Делаем битовый сдвиг (двигаем изображение)
		q = array[8*symbol + j - 1] << 1;
		// Отправляем данные LedMatrix
		sendData(max7219DigitRegisters[j] | q);
		sendData(max7219DigitRegisters[0] | 0);
	}
	if(playSound) return TIME_TO_GO;
	osDelay(SPEED*speed);

	for(uint8_t j = 0; j < 8; j++)
	{
		// Делаем битовый сдвиг (двигаем изображение)
		q = array[8*symbol + j] << 1;
		// Отправляем данные LedMatrix
		sendData(max7219DigitRegisters[j] | q);
	}
	if(playSound) return TIME_TO_GO;
	osDelay(SPEED*speed);

	for(uint8_t j = 0; j < 8; j++)
	{

		// Делаем битовый сдвиг (двигаем изображение)
		q = array[8*symbol + j];
		// Отправляем данные LedMatrix
		sendData(max7219DigitRegisters[j] | q);
	}
	if(playSound) return TIME_TO_GO;
	osDelay(SPEED*speed);
	return NOT_TIME_TO_GO;
}

void Draw(uint8_t* array, int symbol) // Зажигает светодиоиды в LED matrix
{
	sendData(REG_DIGIT_0 | array[8*symbol + 0]);
	sendData(REG_DIGIT_1 | array[8*symbol + 1]);
	sendData(REG_DIGIT_2 | array[8*symbol + 2]);
	sendData(REG_DIGIT_3 | array[8*symbol + 3]);
	sendData(REG_DIGIT_4 | array[8*symbol + 4]);
	sendData(REG_DIGIT_5 | array[8*symbol + 5]);
	sendData(REG_DIGIT_6 | array[8*symbol + 6]);
	sendData(REG_DIGIT_7 | array[8*symbol + 7]);
}

EXIT DrawAll(uint8_t state)
{

	// Пускаем змейку
	for(uint8_t i = 0; i < lenSnake; i++)
	{
		Draw((uint8_t*)snake, i);
		osDelay(SPEED*50);
	}

	for(uint8_t i = 100; i > 1; i-=2)
	{
		if(DrawTS((uint8_t*)symbols, state, i)) return TIME_TO_GO;
	}
	//пускаем анимацию
	switch(state)
	{
	case 1:
		for(uint8_t i = 0; i < lenAnim1; i++)
		{
			Draw((uint8_t*)anim1, i);
			if(playSound) return TIME_TO_GO;
			osDelay(SPEED*50);
		}
		break;

	case 2:
		for(uint8_t i = 0; i < lenAnim2; i++)
		{
			Draw((uint8_t*)anim2, i);
			if(playSound) return TIME_TO_GO;
			osDelay(SPEED*50);
		}
		break;

	case 3:
		for(uint8_t i = 0; i < lenAnim3; i++)
		{
			Draw((uint8_t*)anim3, i);
			if(playSound) return TIME_TO_GO;
			osDelay(SPEED*50);
		}
		break;
	default:
		// Рисуем грустный смайлик
		break;
	}
	// Показываем занятое место
	Draw((uint8_t*)symbols, state);
	// Делаем задержку примерно 10 секунд и параллельно смотрим в очередь
	for(uint8_t i = 0; i!= 100; i++)
	{
		if(playSound) return TIME_TO_GO;
		osDelay(100);
	}
	for(uint8_t i = 100; i > 1; i-=2)
	{
		if(DrawGor((uint8_t*)symbols, state, i)) return TIME_TO_GO;
	}
	// Показываем занятое место
	Draw((uint8_t*)symbols, state);
	// Делаем задержку примерно 10 секунд и параллельно смотрим в очередь
	for(uint8_t i = 0; i!= 100; i++)
	{
		if(playSound) return TIME_TO_GO;
		osDelay(100);
	}
	for(uint8_t i = 100; i > 1; i-=2)
	{
		if(DrawVert((uint8_t*)symbols, state, i)) return TIME_TO_GO;
	}
	// Показываем занятое место
	Draw((uint8_t*)symbols, state);
	// Делаем задержку примерно 10 секунд и параллельно смотрим в очередь
	for(uint8_t i = 0; i!= 100; i++)
	{
		if(playSound) return TIME_TO_GO;
		osDelay(100);
	}
	return NOT_TIME_TO_GO;
}
