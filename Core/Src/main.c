/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "w25qxx_qspi.h"
#include "spi.h"
#include "fatfs.h"
#include <stdio.h>
#include "lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern QSPI_HandleTypeDef hqspi;
#define W25Qxx_TEST              (0)
#define APPLICATION_ADDRESS      QSPI_BASE
#define ISP_ADDRESS              (0x1FF09800UL)
typedef  void (*pFunction)(void);
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
pFunction JumpToApplication;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void CPU_CACHE_Disable(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#if W25Qxx_TEST == (1)
uint8_t read[4096];
uint8_t write[4096];
#endif

uint8_t app_IsReady(uint32_t addr)
{
	if(((*(__IO uint32_t*)addr) & 0x2FF80000 ) == 0x24000000)
		return SUCCESS;
	else if(((*(__IO uint32_t*)addr) & 0x2FF80000 ) == 0x20000000)
		return SUCCESS;
	else
		return ERROR;
}

void app_Jump(uint32_t addr)
{
	pFunction JumpToApplication;
	uint32_t JumpAddress;
	
	/* Jump to user application */
	JumpAddress = *(__IO uint32_t*) (addr + 4);
	JumpToApplication = (pFunction) JumpAddress;

	/* Initialize user application's Stack Pointer */
	__set_MSP(*(__IO uint32_t*) addr);
	JumpToApplication();			
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

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
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  MX_SPI4_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	LCD_Init();
	LCD_ShowString(0,0,160,20,16,(uint8_t*)"bootloader");
	printf("stm32h750vbt6 bootloder start!\r\n");
	/* QPIMode && NormalMode : 100Mhz Clk can run to App, Normal */
	/* QPIMode && DTRMode    : 100Mhz Clk can not run to App, need lower Clk, Recommend 50Mhz */
	/* SPIMode && NormalMode : 100Mhz Clk can run to App, Normal */
	/* SPIMode && DTRMode    : 100Mhz Clk can not run to App, need lower Clk, Recommend 50Mhz*/
	
	/* QPIMode && NormalMode : 120Mhz Clk can not run to App, UnNormal */
	/* QPIMode && DTRMode    : 120Mhz Clk can not run to App, need lower Clk, Recommend 60Mhz */
	/* SPIMode && NormalMode : 120Mhz Clk can run to App, Normal */
	/* SPIMode && DTRMode    : 120Mhz Clk can not run to App, need lower Clk, Recommend 60Mhz*/
	
	w25qxx_Init();
	
	w25qxx_EnterQPI();
	fatfs_test();
#if W25Qxx_TEST == (1)
	W25qxx_EraseSector(0);
	for(uint32_t j=0;j<sizeof(write);j+=256)
		for(uint32_t i=0;i<=0xff;i++)
			write[j+i] = i;
	W25qxx_WriteNoCheck(write,0,sizeof(write));
	W25qxx_Read(read,0,sizeof(read));
#endif
	w25qxx_Startup(w25qxx_DTRMode);
	
#if W25Qxx_TEST == (0)
  /* Reset MCU && Long Press K1 to Enter ISP Mode */
	if(HAL_GPIO_ReadPin(K1_GPIO_Port,K1_Pin) == GPIO_PIN_SET)
	{
		HAL_Delay(50);
		while(HAL_GPIO_ReadPin(K1_GPIO_Port,K1_Pin) == GPIO_PIN_SET)
		{
			HAL_GPIO_TogglePin(PE3_GPIO_Port,PE3_Pin);
			HAL_Delay(80);
		}
		HAL_QSPI_DeInit(&hqspi);
		HAL_GPIO_DeInit(GPIOA,GPIO_PIN_All);
		HAL_GPIO_DeInit(GPIOB,GPIO_PIN_All);
		HAL_GPIO_DeInit(GPIOC,GPIO_PIN_All);
		HAL_GPIO_DeInit(GPIOD,GPIO_PIN_All);
		HAL_GPIO_DeInit(GPIOE,GPIO_PIN_All);
		
		/* Disable CPU L1 cache before jumping to the QSPI code execution */
		CPU_CACHE_Disable();
		/* Disable Systick interrupt */
		SysTick->CTRL = 0;
		
		/* Initialize ISP application's Stack Pointer & Jump to ISP application */
		app_Jump(ISP_ADDRESS);
	}
	
  if(app_IsReady(APPLICATION_ADDRESS) == SUCCESS)
	{
		/* Disable CPU L1 cache before jumping to the QSPI code execution */
		CPU_CACHE_Disable();
		/* Disable Systick interrupt */
		SysTick->CTRL = 0;

		/* Initialize user application's Stack Pointer & Jump to user application */
		app_Jump(APPLICATION_ADDRESS);
	}
#endif	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		//if(HAL_GPIO_ReadPin(K1_GPIO_Port,K1_Pin) == GPIO_PIN_SET) HAL_GPIO_WritePin(PE3_GPIO_Port,PE3_Pin,GPIO_PIN_SET);
		//else HAL_GPIO_WritePin(PE3_GPIO_Port,PE3_Pin,GPIO_PIN_RESET);
		printf("Jump To APP Error!!\r\n");
		HAL_GPIO_TogglePin(PE3_GPIO_Port,PE3_Pin);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 4;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_SPI4
                              |RCC_PERIPHCLK_SPI1|RCC_PERIPHCLK_QSPI;
  PeriphClkInitStruct.PLL2.PLL2M = 2;
  PeriphClkInitStruct.PLL2.PLL2N = 32;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_D1HCLK;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;
  PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
  PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief  CPU L1-Cache disable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Disable(void)
{
  /* Disable I-Cache */
  SCB_DisableICache();

  /* Disable D-Cache */
  SCB_DisableDCache();
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
