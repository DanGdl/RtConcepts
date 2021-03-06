/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os2.h"
#include "stdio.h"
#include <time.h>
#include <stdlib.h>
#include "city_services.h"
#include <string.h>
#include <semphr.h>
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

ETH_HandleTypeDef heth;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for dispatcher */
osThreadId_t dispatcherHandle;
const osThreadAttr_t dispatcher_attributes = {
  .name = "dispatcher",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for dispatcherQueue */
osMessageQueueId_t dispatcherQueueHandle;
const osMessageQueueAttr_t dispatcherQueue_attributes = {
  .name = "dispatcherQueue"
};
/* USER CODE BEGIN PV */
service_attr_t services[TOTAL_SERVICES];

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

#else

#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)

#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE {
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART2 and Loop until the end of transmission */
	HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
	return ch;
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
void StartDefaultTask(void *argument);
void dispatch(void *argument);

/* USER CODE BEGIN PFP */
void serviceTask(void *argument);
void teamTask(void *argument);
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
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of dispatcherQueue */
  dispatcherQueueHandle = osMessageQueueNew (16, sizeof(request_t), &dispatcherQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of dispatcher */
  dispatcherHandle = osThreadNew(dispatch, NULL, &dispatcher_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  srand(time(NULL));
  	for (int i = 0; i < TOTAL_SERVICES; i++) {
  		osMessageQueueAttr_t* serviceQueueAttr = malloc(sizeof(osMessageQueueAttr_t));
  		memset(serviceQueueAttr, 0, sizeof(*serviceQueueAttr));
  		serviceQueueAttr -> name = names[i];
  		osMessageQueueId_t* serviceQueue = osMessageQueueNew(16, sizeof(request_t), serviceQueueAttr);
  		free(serviceQueueAttr);

  		osMutexAttr_t* serviceLockAttributes = malloc(sizeof(osMutexAttr_t));
  		memset(serviceLockAttributes, 0, sizeof(*serviceLockAttributes));
  		serviceLockAttributes -> name = names[i];
  		osMutexId_t serviceLock = osMutexNew(serviceLockAttributes);
  		free(serviceLockAttributes);

  		osSemaphoreAttr_t* semaphoreAttr = malloc(sizeof(osSemaphoreAttr_t));
  		memset(semaphoreAttr, 0, sizeof(*semaphoreAttr));
  		semaphoreAttr -> name = names[i];
  		osSemaphoreId_t semaphore = osSemaphoreNew(teams[i], teams[i], semaphoreAttr);
  		free(semaphoreAttr);

  		services[i].name = names[i];
  		services[i].queue = serviceQueue;
  		services[i].mutex = serviceLock;
  		services[i].semaphore = semaphore;

  		osThreadAttr_t* serviceTaskAttrs = malloc(sizeof(osThreadAttr_t));
  		memset(serviceTaskAttrs, 0, sizeof(*serviceTaskAttrs));
  		serviceTaskAttrs -> name = names[i];
  		serviceTaskAttrs -> stack_size = 128 * 4;
  		serviceTaskAttrs -> priority = (osPriority_t) osPriorityLow;
  		osThreadId_t serviceTaskId = osThreadNew(serviceTask, &services[i], serviceTaskAttrs);
  		free(serviceTaskAttrs);

  		services[i].task = serviceTaskId;
  	}
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void serviceTask(void *argument) {
	osThreadSuspend(osThreadGetId());

	service_attr_t* settings = (service_attr_t*) argument;
	struct request req;
	for(;;) {
		osStatus_t mResult = osMutexAcquire(settings -> mutex, 0);
		if (mResult == osOK) {
			osStatus_t result = osMessageQueueGet(settings -> queue, &req, NULL, 0);
			osMutexRelease(settings -> mutex);

			if (result == osOK) {
				printf("Handle\r\n");

				for (int i = 0; i < req.groups; i++) {
					osThreadAttr_t* teamTaskAttrs = malloc(sizeof(osThreadAttr_t));
					memset(teamTaskAttrs, 0, sizeof(*teamTaskAttrs));
					teamTaskAttrs -> name = NULL;
					teamTaskAttrs -> stack_size = 128 * 4;
					teamTaskAttrs -> priority = (osPriority_t) osPriorityLow;

					task_params_t* params = malloc(sizeof(task_params_t));
					memset(params, 0, sizeof(*params));
					params -> semaphore = settings -> semaphore;
					params -> time = req.task_time;
					params -> name = settings -> name;

					osThreadId_t id = osThreadNew(teamTask, params, teamTaskAttrs);
					if (id == NULL) {
						printf("Failed to launch teamTask\n\r");
						free(params);
					}
					free(teamTaskAttrs);
				}
			} else {
				 // printf("Error receiving message\r\n");
			}
		} else {
			 // printf("Failed to acquire lock\r\n");
		}
		osThreadSuspend(osThreadGetId());
	}
}

void teamTask(void *argument) {
	printf("Exec\n\r");
	task_params_t* params = (task_params_t*) argument;
	for(;;) {
		osStatus_t result = osSemaphoreAcquire(params -> semaphore, 0);
		if (result == osOK) {
			osDelay(pdMS_TO_TICKS(params -> time));
			osSemaphoreRelease(params -> semaphore);
			free(params);
			params = NULL;
			break;
		}
	}
	osThreadExit();
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) {
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for (;;) {
	  int service_num = rand() % TOTAL_SERVICES;
	  int team_num = rand() % 4 + 1;
	  int time_ms = rand() % 10;

	  struct request req = {
			  .service = service_num,
			  .groups = team_num,
			  .task_time = time_ms,
	  };

	  osStatus_t qResult = osMessageQueuePut(dispatcherQueueHandle, &req, 0, 0);
	  if (qResult == osOK) {
		  printf("Generated\r\n");
		  osThreadResume(dispatcherHandle);
	  } else {
		  // printf("Failed to send dispatcher request\r\n");
	  }
	  osDelay(pdMS_TO_TICKS(DELAY));
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_dispatch */
/**
* @brief Function implementing the dispatcher thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_dispatch */
void dispatch(void *argument) {
  /* USER CODE BEGIN dispatch */
  /* Infinite loop */
	osThreadSuspend(osThreadGetId());

	struct request req;
	osStatus_t mResult = osOK;
	osStatus_t putResult = osOK;
	for(;;) {
		osStatus_t getResult = osOK;
		if (mResult == osOK && putResult == osOK) {
			getResult = osMessageQueueGet(dispatcherQueueHandle, &req, NULL, 0);
		}
		if (getResult == osOK) {
		  service_attr_t serviceData = services[req.service];
		  mResult = osMutexAcquire(serviceData.mutex, 0);
		  if (mResult == osOK) {
			  putResult = osMessageQueuePut(serviceData.queue, &req, 0, 0);
			  if (putResult == osOK) {
				  printf("Dispatched\r\n");
			  }
			  osMutexRelease(serviceData.mutex);
			  osThreadResume(serviceData.task);
			  osThreadSuspend(osThreadGetId());
		  } else {
			  // printf("Failed to acquire lock\r\n");
		  }
		} else {
		  // printf("Failed to get message from queue %d\r\n", qResult);
		}
	}
  /* USER CODE END dispatch */
}

 /**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
