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
#include "adc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <cmsis_os2.h>
#include "string.h"
#include "stdio.h"
#include "SEGGER_RTT.h"
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
osThreadId_t app_main_ID, LED_PWM_ID, ADC_Read_ID, PZT_Freq_Check_ID;
uint16_t ADC_Value;
float ADC_Voltage = 0.0f;
float ADC_Current = 0.0f;
char sprintf_buf[48] = {0};

static const osThreadAttr_t ThreadAttr_app_main =
    {
        .name = "app_main",
        .priority = (osPriority_t)osPriorityNormal,
        .stack_size = 256};

static const osThreadAttr_t ThreadAttr_LED_PWM =
    {
        .name = "LED_PWM",
        .priority = (osPriority_t)osPriorityNormal,
        .stack_size = 128};

static const osThreadAttr_t ThreadAttr_ADC_Read =
    {
        .name = "ADC_Read",
        .priority = (osPriority_t)osPriorityNormal,
        .stack_size = 256};

static const osThreadAttr_t ThreadAttr_PZT_Freq_Check =
    {
        .name = "PZT_Freq_Check",
        .priority = (osPriority_t)osPriorityNormal,
        .stack_size = 256};
		
__NO_RETURN void LED_PWM(void *arg)
{
	uint8_t i = 0;
	while(1)
	{
		for(i=0;i<50;i++)
		{
			LL_TIM_OC_SetCompareCH2(TIM1, i);
			osDelay(50);
		}
		osDelay(1000);
		for(i=50;i>0;i--)
		{
			LL_TIM_OC_SetCompareCH2(TIM1, i);
			osDelay(50);
		}
		osDelay(1000);
	}
}

__NO_RETURN void ADC_Read(void *arg)
{
	while(1)
	{
		LL_ADC_REG_StartConversion(ADC1);
		while(LL_ADC_IsActiveFlag_EOS(ADC1) == RESET)
		{
			osDelay(10);
		}
		ADC_Value = LL_ADC_REG_ReadConversionData12(ADC1);
		LL_ADC_ClearFlag_EOS(ADC1);
		
		if(ADC_Value == 0)
		{
			ADC_Value = 1;
		}
		
		ADC_Voltage = 3.3 * (float)ADC_Value / 4096;
		ADC_Current	= ADC_Voltage / 0.25 * 1000;
		if(ADC_Current > 1600) 
		{
			TIM1_StopPWM();
			TIM16_StopPWM();
			SEGGER_RTT_printf(0, "Stop due to Current %.2f mA\r\n", ADC_Current);
//		} 
//		else if(ADC_Current < 20)
//		{
//			LL_TIM_CC_EnableChannel(TIM16, LL_TIM_CHANNEL_CH1);
//			LL_TIM_EnableCounter(TIM16);
//			LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
//			LL_TIM_EnableCounter(TIM1);
		}
		osDelay(25);
	}
}

void PZT_Freq_Check(void *arg)
{
	__IO uint8_t arr = 0, best_arr = 0;
	float max_current = 0.0f, current_now = 0.0f, freq_now = 0.0f;
	SEGGER_RTT_printf(0, "Start PZT_Freq_Check\r\n");
	for(arr = 35; arr <= 40; arr++)
	{
		freq_now = (float)64 / arr;
		LL_TIM_SetAutoReload(TIM16, arr);
		osDelay(100);
		current_now = ADC_Current;
		if(current_now > max_current)
		{
			max_current = current_now;
			best_arr = arr;
		}
		//sprintf(sprintf_buf, "arr:%d ,cur:%.2fmA", arr, current_now);
		//SEGGER_RTT_printf(0, "%s\r\n", sprintf_buf);
	}
	if(max_current < 500) 
	{
		TIM1_StopPWM();
		TIM16_StopPWM();
		SEGGER_RTT_printf(0, "PZT_Freq_Check failed,output stopped!\r\n");
	}
	else
	{
		SEGGER_RTT_printf(0, "PZT_Freq_Check done\r\n");
		//SEGGER_RTT_printf(0, "best_arr:%d, max_current:%.2fmA\r\n", best_arr, max_current);
		LL_TIM_SetAutoReload(TIM16, best_arr);
		LL_TIM_OC_SetCompareCH1(TIM16, best_arr / 2);
	}
}

void app_main(void *arg)
{
	SEGGER_RTT_Init();
	LED_PWM_ID = osThreadNew(LED_PWM, NULL, &ThreadAttr_LED_PWM);
	ADC_Read_ID = osThreadNew(ADC_Read, NULL, &ThreadAttr_ADC_Read);
	PZT_Freq_Check_ID = osThreadNew(PZT_Freq_Check, NULL, &ThreadAttr_PZT_Freq_Check);
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

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, 3);

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	//LL_ADC_DeInit(ADC1);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM16_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	Activate_ADC();
	TIM16_StartPWM();
	TIM1_StartPWM();
	osKernelInitialize();
  app_main_ID = osThreadNew(app_main, NULL, &ThreadAttr_app_main);
  if (osKernelGetState() == osKernelReady)
  {
    osKernelStart();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2)
  {
  }

  /* HSI configuration and activation */
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  /* Main PLL configuration and activation */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 8, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_Enable();
  LL_RCC_PLL_EnableDomain_SYS();
  while(LL_RCC_PLL_IsReady() != 1)
  {
  }

  /* Set AHB prescaler*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

  /* Sysclk activation on the main PLL */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  }

  /* Set APB1 prescaler*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

  LL_Init1msTick(64000000);

  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(64000000);
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
