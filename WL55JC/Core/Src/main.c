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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim16;

UART_HandleTypeDef huart2;

/* Definitions for uart_tx_task */
osThreadId_t uart_tx_taskHandle;
uint32_t uart_tx_taskBuffer[ 128 ];
osStaticThreadDef_t uart_tx_taskControlBlock;
const osThreadAttr_t uart_tx_task_attributes = {
  .name = "uart_tx_task",
  .stack_mem = &uart_tx_taskBuffer[0],
  .stack_size = sizeof(uart_tx_taskBuffer),
  .cb_mem = &uart_tx_taskControlBlock,
  .cb_size = sizeof(uart_tx_taskControlBlock),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for uart_rx_task */
osThreadId_t uart_rx_taskHandle;
uint32_t uart_rx_taskBuffer[ 128 ];
osStaticThreadDef_t uart_rx_taskControlBlock;
const osThreadAttr_t uart_rx_task_attributes = {
  .name = "uart_rx_task",
  .stack_mem = &uart_rx_taskBuffer[0],
  .stack_size = sizeof(uart_rx_taskBuffer),
  .cb_mem = &uart_rx_taskControlBlock,
  .cb_size = sizeof(uart_rx_taskControlBlock),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for sensor_i2c_task */
osThreadId_t sensor_i2c_taskHandle;
uint32_t sensor_i2c_taskBuffer[ 128 ];
osStaticThreadDef_t sensor_i2c_taskControlBlock;
const osThreadAttr_t sensor_i2c_task_attributes = {
  .name = "sensor_i2c_task",
  .stack_mem = &sensor_i2c_taskBuffer[0],
  .stack_size = sizeof(sensor_i2c_taskBuffer),
  .cb_mem = &sensor_i2c_taskControlBlock,
  .cb_size = sizeof(sensor_i2c_taskControlBlock),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for light_adc_task */
osThreadId_t light_adc_taskHandle;
uint32_t light_adc_taskBuffer[ 128 ];
osStaticThreadDef_t light_adc_taskControlBlock;
const osThreadAttr_t light_adc_task_attributes = {
  .name = "light_adc_task",
  .stack_mem = &light_adc_taskBuffer[0],
  .stack_size = sizeof(light_adc_taskBuffer),
  .cb_mem = &light_adc_taskControlBlock,
  .cb_size = sizeof(light_adc_taskControlBlock),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for servo_task */
osThreadId_t servo_taskHandle;
uint32_t servo_taskBuffer[ 128 ];
osStaticThreadDef_t servo_taskControlBlock;
const osThreadAttr_t servo_task_attributes = {
  .name = "servo_task",
  .stack_mem = &servo_taskBuffer[0],
  .stack_size = sizeof(servo_taskBuffer),
  .cb_mem = &servo_taskControlBlock,
  .cb_size = sizeof(servo_taskControlBlock),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for serial_tx_data_queue */
osMessageQueueId_t serial_tx_data_queueHandle;
uint8_t serial_tx_data_queueBuffer[ 16 * sizeof( uint64_t ) ];
osStaticMessageQDef_t serial_tx_data_queueControlBlock;
const osMessageQueueAttr_t serial_tx_data_queue_attributes = {
  .name = "serial_tx_data_queue",
  .cb_mem = &serial_tx_data_queueControlBlock,
  .cb_size = sizeof(serial_tx_data_queueControlBlock),
  .mq_mem = &serial_tx_data_queueBuffer,
  .mq_size = sizeof(serial_tx_data_queueBuffer)
};
/* Definitions for event_flags */
osEventFlagsId_t event_flagsHandle;
osStaticEventGroupDef_t event_flagsControlBlock;
const osEventFlagsAttr_t event_flags_attributes = {
  .name = "event_flags",
  .cb_mem = &event_flagsControlBlock,
  .cb_size = sizeof(event_flagsControlBlock),
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM16_Init(void);
void uart_tx_task_routine(void *argument);
void uart_rx_task_routine(void *argument);
void sensor_i2c_task_routine(void *argument);
void light_adc_task_routine(void *argument);
void servo_task_routine(void *argument);

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
  MX_ADC_Init();
  MX_I2C2_Init();
  MX_USART2_UART_Init();
  MX_TIM16_Init();
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
  /* creation of serial_tx_data_queue */
  serial_tx_data_queueHandle = osMessageQueueNew (16, sizeof(uint64_t), &serial_tx_data_queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of uart_tx_task */
  uart_tx_taskHandle = osThreadNew(uart_tx_task_routine, NULL, &uart_tx_task_attributes);

  /* creation of uart_rx_task */
  uart_rx_taskHandle = osThreadNew(uart_rx_task_routine, NULL, &uart_rx_task_attributes);

  /* creation of sensor_i2c_task */
  sensor_i2c_taskHandle = osThreadNew(sensor_i2c_task_routine, NULL, &sensor_i2c_task_attributes);

  /* creation of light_adc_task */
  light_adc_taskHandle = osThreadNew(light_adc_task_routine, NULL, &light_adc_task_attributes);

  /* creation of servo_task */
  servo_taskHandle = osThreadNew(servo_task_routine, NULL, &servo_task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of event_flags */
  event_flagsHandle = osEventFlagsNew(&event_flags_attributes);

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK3Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.NbrOfConversion = 1;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_160CYCLES_5;
  hadc.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_160CYCLES_5;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00000003;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 2;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 40000-1;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 2000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim16, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim16, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */
  HAL_TIM_MspPostInit(&htim16);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15|GPIO_PIN_9|GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB15 PB9 PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_15|GPIO_PIN_9|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void uart_rx_task_pre_loop(void);
void sensor_i2c_task_pre_loop(void);
void light_adc_task_pre_loop(void);

void uart_tx_task_iteration(void);
void uart_rx_task_iteration(void);
void sensor_i2c_task_iteration(void);
void light_adc_task_iteration(void);
void servo_task_iteration(void);

/* USER CODE END 4 */

/* USER CODE BEGIN Header_uart_tx_task_routine */
/**
  * @brief  Function implementing the uart_tx_task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_uart_tx_task_routine */
void uart_tx_task_routine(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    uart_tx_task_iteration();
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_uart_rx_task_routine */
/**
* @brief Function implementing the uart_rx_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_uart_rx_task_routine */
void uart_rx_task_routine(void *argument)
{
  /* USER CODE BEGIN uart_rx_task_routine */
  uart_rx_task_pre_loop();
  /* Infinite loop */
  for(;;)
  {
	uart_rx_task_iteration();
    osDelay(1);
  }
  /* USER CODE END uart_rx_task_routine */
}

/* USER CODE BEGIN Header_sensor_i2c_task_routine */
/**
* @brief Function implementing the sensor_i2c_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_sensor_i2c_task_routine */
void sensor_i2c_task_routine(void *argument)
{
  /* USER CODE BEGIN sensor_i2c_task_routine */
  sensor_i2c_task_pre_loop();
  /* Infinite loop */
  for(;;)
  {
    sensor_i2c_task_iteration();
    osDelay(1);
  }
  /* USER CODE END sensor_i2c_task_routine */
}

/* USER CODE BEGIN Header_light_adc_task_routine */
/**
* @brief Function implementing the light_adc_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_light_adc_task_routine */
void light_adc_task_routine(void *argument)
{
  /* USER CODE BEGIN light_adc_task_routine */
  light_adc_task_pre_loop();
  /* Infinite loop */
  for(;;)
  {
	light_adc_task_iteration();
    osDelay(100); // 10Hz
  }
  /* USER CODE END light_adc_task_routine */
}

/* USER CODE BEGIN Header_servo_task_routine */
/**
* @brief Function implementing the servo_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_servo_task_routine */
void servo_task_routine(void *argument)
{
  /* USER CODE BEGIN servo_task_routine */
  /* Infinite loop */

  for(;;)
  {
    osDelay(100000);
  }
  /* USER CODE END servo_task_routine */
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
