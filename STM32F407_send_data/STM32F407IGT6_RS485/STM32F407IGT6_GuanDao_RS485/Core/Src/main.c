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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/*USART号、时钟、波特率*/
#define _485_USART                             UART4
#define _485_USART_CLK_ENABLE()                __UART4_CLK_ENABLE();
#define _485_USART_BAUDRATE                    115200

#define RCC_PERIPHCLK_485_USART                RCC_PERIPHCLK_UART4
#define RCC_485_USARTCLKSOURCE_SYSCLK          RCC_USART4CLKSOURCE_SYSCLK
/*RX引脚*/
#define _485_USART_RX_GPIO_PORT                GPIOC
#define _485_USART_RX_GPIO_CLK_ENABLE()        __GPIOC_CLK_ENABLE()
#define _485_USART_RX_PIN                      GPIO_PIN_11
#define _485_USART_RX_AF                       GPIO_AF8_UART4
/*TX引脚*/
#define _485_USART_TX_GPIO_PORT                GPIOC
#define _485_USART_TX_GPIO_CLK_ENABLE()        __GPIOC_CLK_ENABLE()
#define _485_USART_TX_PIN                      GPIO_PIN_10
#define _485_USART_TX_AF                       GPIO_AF8_UART4
/*485收发控制引脚*/
#define _485_RE_GPIO_PORT					   GPIOH
#define _485_RE_GPIO_CLK_ENABLE()              __GPIOH_CLK_ENABLE()
#define _485_RE_PIN							   GPIO_PIN_9
/*中断相关*/
#define _485_INT_IRQ                 		   UART4_IRQn
#define bsp_485_IRQHandler                     UART4_IRQHandler

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// 不精确的延时
static void _485_delay(__IO uint32_t nCount)
{
	for(; nCount != 0; nCount--);
}
/*控制收发引脚*/
//进入接收模式,必须要有延时等待485处理完数据
#define _485_RX_EN()			_485_delay(1000);\
		HAL_GPIO_WritePin(_485_RE_GPIO_PORT,_485_RE_PIN,GPIO_PIN_SET); _485_delay(1000);
//进入发送模式,必须要有延时等待485处理完数据
#define _485_TX_EN()			_485_delay(1000);\
		HAL_GPIO_WritePin(_485_RE_GPIO_PORT,_485_RE_PIN,GPIO_PIN_RESET); _485_delay(1000);

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

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
		uint8_t GuanDao_Data[110] ={0xAA, 0x55, 0x5A, 0xA5, 0x4A, 0x6E, 0xAA, 0xAA, 0xAA, 0xAA, 0xBB, 0xBB, 0xBB, 0xBB, 0x1D, 0x3E, 0x00, 0x00, 0x00, 
																0x00, 0xE2, 0x01, 0x8C, 0x00, 0xEA, 0x56, 0xCC, 0xCC, 0xDD, 0xDD, 0xEE, 0xEE, 0x10, 0x52, 0x67, 0x0A, 0xB3, 0x1A, 
																0x5E, 0x4A, 0xD2, 0x56, 0x1A, 0x3E, 0x00, 0x00, 0xF7, 0x01, 0x05, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
																0x00, 0x00, 0x00, 0x00, 0x00, 0x93, 0x30, 0xF7, 0xFF, 0xF4, 0x30, 0x69, 0x00, 0x52, 0xA7, 0xFE, 0xFF, 0xF9, 0x6A, 
																0x19, 0x00, 0x64, 0x83, 0x21, 0x00, 0xDB, 0x65, 0x1E, 0xFA, 0x23, 0x07, 0x00, 0x34, 0xFF, 0xFF, 0xFF, 0x11, 0x11, 
																0x11, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x24};
		uint8_t CX_Data[60] ={0x4C, 0x57, 0x3C, 0x00, 0x01, 0x00, 0x01, 0x01, 
													0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
													0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
													0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
													0x00, 0x00, 0x0D, 0x0A};
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
  MX_UART4_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_Delay(1000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	
  while (1)
  {
    uint8_t key = ScanKey();
		SetLeds(key);
		//if(key)
			//{
				//printf("%02X\n",key);
				//HAL_Delay(100);
			//}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	
		_485_TX_EN();
		HAL_UART_Transmit(&huart4, (uint8_t *)&CX_Data, 60, 0xffff);
		//for(int i = 0; i < 110; i ++)
		//{
			//HAL_UART_Transmit(&huart4, (uint8_t *)&GuanDao_Data[i], 1, 0xffff);
		//}
		
		HAL_UART_Transmit(&huart1, (uint8_t *)&GuanDao_Data, 110, 0xffff);
 
		_485_RX_EN();
		HAL_Delay(1000);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
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
