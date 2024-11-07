/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t calChecksum(uint8_t* str2, int size)
{
    uint16_t sum = 0;
    for(int i = 0; i < size; i ++)
    {
        sum += str2[i];
    }

    return sum;
}

// 
uint32_t bytesToUint32(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) 
{
    return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}

// 
uint16_t bytesToUint16(uint8_t b0, uint8_t b1) 
{
    return (b1 << 8) | b0;
}

uint32_t bytesToInt(uint8_t b0, uint8_t b1, uint8_t b2)
{
	uint32_t year = b2 * 10000;
	uint16_t month = b1 * 100;
	return (year + month + b0);
}

	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sData;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	/*
	uint8_t str[110] ={0XAA,0X55,0X5A,0XA5,0xAA,0x6E,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x20,0x21,0x22,0x23,0x24,0x25,0x26,
										 0x27,0x28,0x29,0x30, 0x31	,0x32	,0x33	,0x34	,0x35	,0x36	,0x37	,0x38	,0x39	,0x40	,0x41	,0x42	,0x43	,0x44	,0x45	,0x46	,0x47	,0x48	,0x49	,
										 0x50	,0x51	,0x52	,0x53	,0x54	,0x55	,0x56	,0x57	,0x58	,0x59	,0x60,0x61	,0x62	,0x63	,0x64	,0x65	,0x66	,0x67	,0x68	,0x69	,0x70	,0x71	,0x72	,
										 0x73	,0x74	,0x75	,0x76	,0x77	,0x78	,0x79	,0x80	,0x81	,0x82	,0x83	,0x84	,0x85	,0x86	,0x87	,0x88	,0x89,0x90	,0x91	,0x92	,0x93	,0x94	,0x95	,
											0x96	,0x97	,0x98	,0x99	,0xA0	,0xA1	,0xA2	,0xA3	,0xA4	,0xA5	,0xA6	,0xA7	,0xA8	,0xA9};
	*/
	uint8_t str2[110] ={0xAA, 0x55, 0x5A, 0xA5, 0x4A, 0x6E, 0x6E, 0x0A, 0xB3, 0x1A, 0x60, 0x4A, 0xD2, 0x56, 0x1D, 0x3E, 0x00, 0x00, 0x00, 
											0x00, 0xE2, 0x01, 0x8C, 0x00, 0xEA, 0xFF, 0x1E, 0xFE, 0xDB, 0x01, 0x64, 0x0B, 0x10, 0x52, 0x67, 0x0A, 0xB3, 0x1A, 
											0x5E, 0x4A, 0xD2, 0x56, 0x1A, 0x3E, 0x00, 0x00, 0xF7, 0x01, 0x05, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
											0x00, 0x00, 0x00, 0x00, 0x00, 0x93, 0x30, 0xF7, 0xFF, 0xF4, 0x30, 0x69, 0x00, 0x52, 0xA7, 0xFE, 0xFF, 0xF9, 0x6A, 
											0x19, 0x00, 0x64, 0x83, 0x21, 0x00, 0xDB, 0x65, 0x1E, 0xFA, 0x23, 0x07, 0x00, 0x00, 0x12, 0xAB, 0x03, 0x36, 0x4F, 
											0x01, 0x3A, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x1E};
	//uint8_t YY;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	//RTC_DateTypeDef sData2;
	//RTC_TimeTypeDef sTime2;
	
	sData.Date = 2;
	sData.Month = RTC_MONTH_JULY;
	sData.Year = 24;

	sTime.Seconds = 10;
	sTime.Minutes = 10;
	sTime.Hours = 15;
	sTime.TimeFormat = RTC_HOURFORMAT_24;
	if (HAL_RTC_SetDate(&hrtc, &sData, RTC_FORMAT_BIN) != HAL_OK) {
        // 
        Error_Handler();
    }
	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
        // 
        Error_Handler();
    }
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		

		
		HAL_StatusTypeDef StatTIme =HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
		HAL_StatusTypeDef StatData = HAL_RTC_GetDate(&hrtc, &sData, RTC_FORMAT_BIN);
		

		if(StatData == HAL_OK)
		{
			
			uint32_t Data = bytesToInt(sData.Date, sData.Month, sData.Year);
			
			str2[90] = Data & 0xFF;
			str2[91] = (Data >> 8) & 0xFF;
			str2[92] = (Data >> 16) & 0xFF;
		}else{
			str2[90] = 0x00;
			str2[91] = 0x00;
			str2[92] = 0x00;
		}
		if(StatTIme == HAL_OK)
		{
			uint32_t Time = bytesToInt(sTime.Seconds, sTime.Minutes, sTime.Hours);
			str2[93] = Time & 0xFF;
			str2[94] = (Time >> 8) & 0xFF;
			str2[95] = (Time >> 16) & 0xFF;
			
		}else{
			str2[93] = 0x00;
			str2[94] = 0x00;
			str2[95] = 0x00;
		}
	
		
		uint16_t sum = calChecksum(str2,108);
		str2[108] = 0xFF & sum;
		str2[109] = (sum >> 8) & 0xFF;
		
		HAL_UART_Transmit(&huart1, (uint8_t *)str2, sizeof(str2), 0xffff);
		/*
		for(int i = 0; i < 110; i ++)
		{
			HAL_UART_Transmit(&huart1, (uint8_t *)&str2[i], 1, 0xffff);
		}
		*/
		HAL_Delay(900);

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
