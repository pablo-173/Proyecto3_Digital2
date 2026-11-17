/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <Neopixel.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    NP_REPOSO,
    NP_ADQUIRIENDO,
    NP_TRANSMITIENDO,
    NP_GUARDANDO
} EstadoNeoPixel;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TIM_FREQ 84000000
#define PSC 0 //1
#define PI 3.1415926
#define size 128
#define UART3_RX_TAM 512
#define NP_TIEMPO_TX 500U
#define NP_TIMEOUT 15000U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim6;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
static EstadoNeoPixel estadoNP = NP_REPOSO;
static uint8_t operacionNP = 0;
static uint32_t inicioTX = 0;
static uint32_t ultimaActividadNP = 0;
uint32_t YSine[size];

uint8_t comandoLEER[] = {0x4C, 0x45, 0x45, 0x52, 0x0D, 0x0A};
uint8_t comandoGUARDAR[] = {0x47, 0x55, 0x41, 0x52, 0x44, 0x41, 0x52, 0x0D, 0x0A};
uint8_t comandoVER[] = {0x56, 0x45, 0x52, 0x0D, 0x0A};
uint8_t comandoESTADO[] = {0x45, 0x53, 0x54, 0x41, 0x44, 0x4F, 0x0D, 0x0A};

static uint8_t uart3RxByte;


static volatile uint16_t uart3Head = 0;
static volatile uint16_t uart3Tail = 0;
static uint32_t uart3UltimoByte = 0;
static volatile uint32_t uart3Overflow = 0;
static volatile uint32_t uart3BytesRecibidos = 0;

static uint16_t uart3Pos = 0;


volatile uint8_t comandoRecibido = 0;
volatile uint8_t comandoPendiente=0;
int antiR=0;
uint32_t lecPot1;
uint32_t lecPot1_Ant=0;
uint8_t dato=0;
volatile uint8_t datoRecibido = 0;
volatile uint8_t solicitarlectura=0;
char mensaje[160];
volatile uint8_t Sonido=0;
// Sonido de inicio: tres notas ascendentes
uint16_t sonidoInicio[] = {392, 523, 659};
uint16_t duracionInicio[] = {100, 100, 180};

// Sonido de finalizacion: dos notas de confirmacion
uint16_t sonidoFinal[] = {784, 1047};
uint16_t duracionFinal[] = {120, 220};
uint16_t sonidoError[] = {880, 440, 220};
uint16_t duracionError[] = {110, 130, 240};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM6_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */
void lectura(void);

void generarSin(void);
int calcularARR(int freq);
void playTone(const uint16_t *tone, const uint16_t *duration, int Nsize, const uint16_t *pause);
void noTone(void);
void generarSonido(void);
void ProcesarComandoEsp32(uint8_t);
void Detener(void);
void UART3_ProcesarRecibidos(void);
void ProcesarRespuestaNucleo2(const char *respuesta);
static uint8_t uart3Buffer[UART3_RX_TAM];
void NeoPixel_CambiarEstado(EstadoNeoPixel estado);
void NeoPixel_Respuesta(const char *respuesta);
void NeoPixel_Actualizar(void);
void Sonido_Inicio(void);
void Sonido_Final(void);
void Sonido_Error(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void generarSin(void) {
    for (int x = 0; x < size; x++) {
        YSine[x] = ((sin(x * 2 * PI / size) + 1) * (4096 / 2));
    }
}
int calcularARR(int freq) {
    if (freq == 0)
        return 0;

    int TF = size * freq;
    return ((TIM_FREQ / ((PSC + 1) * TF)) - 1);
}

void playTone(const uint16_t *tone, const uint16_t *duration, int Nsize, const uint16_t *pause)
{
    for (int i = 0; i < Nsize; i++)
    {
        int valorARR = calcularARR(tone[i]);  // calcular el valor ARR
        int dur = duration[i];                // obtener la duración
        int pauseBetweenTones = 0;

        if (pause != NULL)
            pauseBetweenTones = pause[i] - duration[i];

        TIM6->ARR = valorARR;
        TIM6->CNT = 0;
        HAL_TIM_Base_Start(&htim6);
        HAL_Delay(dur);                       // duración nota
        noTone();                             // pausa
        HAL_Delay(pauseBetweenTones);         // duración sin tono
    }
}
void noTone(void)
{
    HAL_TIM_Base_Stop(&htim6);
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
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_TIM6_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, (uint8_t*)&dato,1);
  generarSin();

  HAL_DAC_Init(&hdac);
  HAL_TIM_Base_Start(&htim6);


  HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, YSine, size, DAC_ALIGN_12B_R);

  noTone();
  HAL_UART_Receive_IT(&huart3, &uart3RxByte, 1);

  setBrightness(50);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  UART3_ProcesarRecibidos();
	  NeoPixel_Actualizar();
	  if(comandoPendiente){
		  comandoPendiente=0;
		  ProcesarComandoEsp32(dato);
	  }

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 105-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 1-1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 1000-1;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Bt1_Pin */
  GPIO_InitStruct.Pin = Bt1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(Bt1_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */



void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        switch (dato)
        {
            case '1':  // LEER
                comandoPendiente = 1;
                Sonido = 1;
                break;

            case '2':  // GUARDAR
                comandoPendiente = 1;
                Sonido = 2;
                break;

            case '3':  // VER
                comandoPendiente = 1;
                Sonido = 4;
                break;

            case '4':  // ESTADO
                comandoPendiente = 1;
                Sonido = 3;
                break;
        }

        // Reactivar recepción por interrupción
        HAL_UART_Receive_IT(&huart1, &dato, 1);
    }
    else if (huart->Instance == UART5)
    {
        uint16_t siguiente =
            (uart3Head + 1) % UART3_RX_TAM;

        if (siguiente != uart3Tail)
        {
            uart3Buffer[uart3Head] = uart3RxByte;
            uart3Head = siguiente;
        }
        else
        {
            uart3Overflow++;
        }

        HAL_UART_Receive_IT(&huart3, &uart3RxByte, 1);
    }
}



void ProcesarComandoEsp32(uint8_t comando)
{
	if (comando < '1' || comando > '4')
	        return;

	    operacionNP = comando;
	    ultimaActividadNP = HAL_GetTick();
	    Sonido_Inicio();
    switch (comando)
    {
        case '1':
        	 NeoPixel_CambiarEstado(NP_ADQUIRIENDO);
 // Verde
			pixelShow();
            HAL_UART_Transmit(&huart2,
                (uint8_t*)"LEER\r\n", 6, 100);
            HAL_UART_Transmit(&huart5, comandoLEER, sizeof(comandoLEER), 100);
            break;

        case '2':
        	NeoPixel_CambiarEstado(NP_GUARDANDO);
			pixelShow();
            HAL_UART_Transmit(&huart2,
                (uint8_t*)"GUARDAR\r\n", 9, 100);
            HAL_UART_Transmit(&huart5, comandoGUARDAR, sizeof(comandoGUARDAR), 100);
            break;

        case '3':
        	 NeoPixel_CambiarEstado(NP_TRANSMITIENDO);
			pixelShow();
            HAL_UART_Transmit(&huart2,
                (uint8_t*)"VER\r\n", 5, 100);
            HAL_UART_Transmit(&huart5, comandoVER, sizeof(comandoVER), 100);
            break;

        case '4':
        	NeoPixel_CambiarEstado(NP_TRANSMITIENDO); // Verde
			pixelShow();
            HAL_UART_Transmit(&huart5,
                (uint8_t*)"ESTADO\r\n", 8, 100);
            HAL_UART_Transmit(&huart5, comandoESTADO, sizeof(comandoESTADO), 100);
            break;
    }
}

void generarSonido(){
	switch (Sonido){
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	}
}

void UART3_ProcesarRecibidos(void)
{
    // Recibir bytes sin depender de USART3_IRQHandler ni NVIC.
    // El while principal ya llama a esta funcion continuamente.
    while (HAL_UART_Receive(&huart5, &uart3RxByte, 1, 0) == HAL_OK)
    {
        uart3BytesRecibidos++;
        if (uart3Pos < UART3_RX_TAM)
        {
            uart3Buffer[uart3Pos++] = uart3RxByte;
        }
        else
        {
            uart3Overflow++;
        }
        uart3UltimoByte = HAL_GetTick();
    }

    // Recuperacion si el receptor tuvo un overrun.
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_ORE) != RESET)
    {
        __HAL_UART_CLEAR_OREFLAG(&huart3);
        uart3Overflow++;
    }

    // Nucleo 2 manda mensajes cortos y bloques SD separados por pausas.
    // Esperar un breve silencio y reenviar el bloque completo por USART2.
    if (uart3Pos > 0 &&
        (uint32_t)(HAL_GetTick() - uart3UltimoByte) >= 3U)
    {
        uart3Buffer[uart3Pos] = '\0';

        HAL_UART_Transmit(
            &huart2,
            uart3Buffer,
            uart3Pos,
            1000
        );

        NeoPixel_Respuesta((const char *)uart3Buffer);

        uart3Pos = 0;
    }   if (uart3Pos > 0 &&
        (uint32_t)(HAL_GetTick() - uart3UltimoByte) >= 3U)
    {
        HAL_UART_Transmit(&huart2, uart3Buffer, uart3Pos, 1000);
        uart3Pos = 0;
    }
}
void NeoPixel_CambiarEstado(EstadoNeoPixel estado)
{
    estadoNP = estado;

    switch (estado)
    {
        case NP_REPOSO:
            setPixelColor(0, 255, 255, 255); // Blanco
            break;

        case NP_ADQUIRIENDO:
            setPixelColor(0, 0, 0, 255); // Azul
            break;

        case NP_TRANSMITIENDO:
            setPixelColor(0, 204, 39, 245); // Amarillo
            break;

        case NP_GUARDANDO:
            setPixelColor(0, 0, 255, 0); // Verde
            break;
    }

    pixelShow();
}

void NeoPixel_Respuesta(const char *respuesta)
{
    // Detectar errores enviados por Nucleo 2
    uint8_t esError =
        (strstr(respuesta, "ERROR") != NULL) ||
        (strstr(respuesta, "COMANDO_DESCONOCIDO") != NULL);

    // Error de montaje SD al iniciar el sistema
    if (operacionNP == 0)
    {
        if (strstr(respuesta, "ERROR AL MONTAR SD") != NULL)
        {
            Sonido_Error();
        }

        return;
    }

    ultimaActividadNP = HAL_GetTick();

    uint8_t terminado = esError;

    switch (operacionNP)
    {
        case '1': // LEER

            if (strstr(respuesta, "MEDICION:") != NULL)
            {
                terminado = 1;
            }
            break;

        case '2': // GUARDAR

            if (strstr(respuesta, "GUARDADO_OK") != NULL)
            {
                terminado = 1;
            }
            break;

        case '3': // VER

            if (strstr(respuesta, "VER_OK") != NULL)
            {
                terminado = 1;
            }
            break;

        case '4': // ESTADO

            if (strstr(respuesta, "MEDICION_DISPONIBLE") != NULL ||
                strstr(respuesta, "SIN_MEDICION") != NULL)
            {
                terminado = 1;
            }
            break;
    }

    if (terminado)
    {
        operacionNP = 0;

        NeoPixel_CambiarEstado(NP_TRANSMITIENDO);

        inicioTX = HAL_GetTick();

        if (esError)
        {
            Sonido_Error();
        }
        else
        {
            Sonido_Final();
        }
    }
}

void NeoPixel_Actualizar(void)
{
    uint32_t ahora = HAL_GetTick();

    // Regresar a reposo 500 ms despues
    // de terminar la transmision
    if (estadoNP == NP_TRANSMITIENDO &&
        operacionNP == 0)
    {
        if ((uint32_t)(ahora - inicioTX) >= 500)
        {
            Detener();
        }
    }

    // Si Nucleo 2 no responde en 15 segundos
    if (operacionNP != 0 &&
        (uint32_t)(ahora - ultimaActividadNP) >= 15000)
    {
        HAL_UART_Transmit(
            &huart2,
            (uint8_t*)"TIMEOUT NUCLEO 2\r\n",
            sizeof("TIMEOUT NUCLEO 2\r\n") - 1,
            100
        );
Sonido_Error();

        Detener();
    }
}
void Sonido_Inicio(void)
{
    playTone(sonidoInicio, duracionInicio, 3, NULL);
}

void Sonido_Final(void)
{
    playTone(sonidoFinal, duracionFinal, 2, NULL);
}
void Sonido_Error(void)
{
    playTone(sonidoError, duracionError, 3, NULL);
}
void Detener(){




	    operacionNP = 0;

	    NeoPixel_CambiarEstado(NP_REPOSO);

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
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
