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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9341.h"
#include "Bitmaps.h"
#include "fatfs.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define COLOR_FONDO    0x0842
#define COLOR_PANEL    0x10C4
#define COLOR_BARRA    0x2947
#define COLOR_CIAN     0x07FF
#define COLOR_BLANCO   0xFFFF
#define COLOR_GRIS     0xAD55
#define COLOR_VERDE    0x07E0
#define COLOR_AMARILLO 0xFFE0
#define COLOR_ROJO     0xF800

#define LUX_MAXIMO     9999
#define BH1750_ADDR (0x23 << 1)
#define BH1750_MODO_CONTINUO 0x10

/* Control de mediciones */
static uint8_t medicionValida = 0;
static uint16_t ultimaMedicion = 0;

/* Recepcion de comandos UART */
static char comandoUART[24];
static uint8_t indiceUART = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
extern uint16_t fondo[];
int Mluz = 0;

/* Variables para prueba SD */
FATFS fs;
FRESULT resultadoSD;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


void DibujarDigito(int numero, int x, int y,
                   uint16_t color)
{
    const uint8_t segmentos[10] = {
        0x3F, // 0
        0x06, // 1
        0x5B, // 2
        0x4F, // 3
        0x66, // 4
        0x6D, // 5
        0x7D, // 6
        0x07, // 7
        0x7F, // 8
        0x6F  // 9
    };

    if (numero < 0 || numero > 9)
        return;

    uint8_t s = segmentos[numero];

    // Segmento A - superior
    if (s & 0x01)
        FillRect(x+5, y, 30, 6, color);

    // Segmento B - superior derecho
    if (s & 0x02)
        FillRect(x+34, y, 6, 28, color);

    // Segmento C - inferior derecho
    if (s & 0x04)
        FillRect(x+34, y+27, 6, 28, color);

    // Segmento D - inferior
    if (s & 0x08)
        FillRect(x+5, y+49, 30, 6, color);

    // Segmento E - inferior izquierdo
    if (s & 0x10)
        FillRect(x, y+27, 6, 28, color);

    // Segmento F - superior izquierdo
    if (s & 0x20)
        FillRect(x, y, 6, 28, color);

    // Segmento G - central
    if (s & 0x40)
        FillRect(x+5, y+25, 30, 6, color);
}



void MostrarLux(uint16_t lux)
{
    // Limpiar el area de los numeros
    FillRect(22, 75, 200, 60, COLOR_PANEL);

    // Limitar a 9999 lux
    if (lux > 9999)
    {
        lux = 9999;
    }

    // Separar los cuatro digitos
    int millares = lux / 1000;
    int centenas = (lux / 100) % 10;
    int decenas = (lux / 10) % 10;
    int unidades = lux % 10;

    // Dibujar millares
    if (lux >= 1000)
    {
        DibujarDigito(millares, 25, 77,
                      COLOR_CIAN);
    }

    // Dibujar centenas
    if (lux >= 100)
    {
        DibujarDigito(centenas, 75, 77,
                      COLOR_CIAN);
    }

    // Dibujar decenas
    if (lux >= 10)
    {
        DibujarDigito(decenas, 125, 77,
                      COLOR_CIAN);
    }

    // Dibujar unidades
    DibujarDigito(unidades, 175, 77,
                  COLOR_CIAN);
}


static void LuxMeter_DibujarBase(void)
{
    LCD_Clear(COLOR_FONDO);

    // Encabezado
    FillRect(0, 0, 320, 36, COLOR_PANEL);
    FillRect(0, 35, 320, 2, COLOR_CIAN);

    LCD_Print("LUXMETER PRO", 12, 10, 2,
              COLOR_BLANCO, COLOR_PANEL);

    LCD_Print("STM32", 264, 13, 1,
              COLOR_CIAN, COLOR_PANEL);

    // Area principal
    LCD_Print("INTENSIDAD LUMINICA", 18, 49, 1,
              COLOR_GRIS, COLOR_FONDO);

    // Panel del valor
    FillRect(16, 70, 288, 66, COLOR_PANEL);

    // Panel de nivel
    LCD_Print("NIVEL DE LUZ", 18, 150, 1,
              COLOR_GRIS, COLOR_FONDO);

    // Fondo de la barra
    FillRect(18, 174, 284, 14, COLOR_BARRA);

    LCD_Print("0 LUX", 18, 195, 1,
              COLOR_GRIS, COLOR_FONDO);

    LCD_Print("1000 LUX", 250, 195, 1,
              COLOR_GRIS, COLOR_FONDO);

    // Pie de pantalla
    FillRect(0, 218, 320, 22, COLOR_PANEL);

    FillRect(12, 225, 7, 7, COLOR_VERDE);

    LCD_Print("SENSOR ACTIVO", 25, 224, 1,
              COLOR_BLANCO, COLOR_PANEL);

    LCD_Print("ONLINE", 267, 224, 1,
              COLOR_VERDE, COLOR_PANEL);
}

static void LuxMeter_Actualizar(uint16_t lux)
{
    char texto[20];
    uint16_t colorNivel;
    const char *estado;
    uint16_t ancho;

    // Clasificacion visual
    if (lux < 100)
    {
        estado = "BAJO";
        colorNivel = COLOR_GRIS;
    }
    else if (lux < 500)
    {
        estado = "NORMAL";
        colorNivel = COLOR_CIAN;
    }
    else if (lux < 2000)
    {
        estado = "ALTO";
        colorNivel = COLOR_VERDE;
    }
    else
    {
        estado = "PELIGROSO";
        colorNivel = COLOR_AMARILLO;
    }

    // Actualizar valor numerico
    MostrarLux(lux);


    LCD_Print("LUX", 229, 96, 2,
              COLOR_CIAN, COLOR_PANEL);

    // Actualizar estado
    FillRect(220, 145, 84, 21, COLOR_FONDO);

    LCD_Print((char *)estado, 228, 151, 1,
              colorNivel, COLOR_FONDO);

    // Actualizar barra
    FillRect(18, 174, 284, 14, COLOR_BARRA);

    uint16_t valorBarra = lux;

    if (valorBarra > LUX_MAXIMO)
    {
        valorBarra = LUX_MAXIMO;
    }

    ancho = (uint16_t)
            (((uint32_t)valorBarra * 284)
            / LUX_MAXIMO);

    if (ancho > 0)
    {
        FillRect(18, 174, ancho, 14,
                 colorNivel);
    }
}


static HAL_StatusTypeDef BH1750_Iniciar(void)
{
    uint8_t comando = BH1750_MODO_CONTINUO;

    if (HAL_I2C_IsDeviceReady(
            &hi2c1, BH1750_ADDR, 3, 100) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_I2C_Master_Transmit(
            &hi2c1, BH1750_ADDR,
            &comando, 1, 100) != HAL_OK)
    {
        return HAL_ERROR;
    }

    HAL_Delay(180);
    return HAL_OK;
}

static HAL_StatusTypeDef BH1750_Leer(void)
{
    uint8_t datos[2];

    if (HAL_I2C_Master_Receive(
            &hi2c1, BH1750_ADDR,
            datos, 2, 100) != HAL_OK)
    {
        return HAL_ERROR;
    }

    uint16_t raw =
        ((uint16_t)datos[0] << 8) | datos[1];

    // Guardar intensidad luminica en Mluz
    Mluz = (9999*(uint32_t)raw)/65535;

    return HAL_OK;
}


static FRESULT SD_GuardarMedicion(void)
{
    FIL archivo;
    FRESULT res;
    FRESULT cierre;
    UINT escritos = 0;
    char registro[40];

    if (!medicionValida)
    {
        return FR_INVALID_OBJECT;
    }

    int longitud = snprintf(
        registro,
        sizeof(registro),
        "%u,lux\r\n",
        (unsigned int)ultimaMedicion
    );

    if (longitud <= 0 ||
        longitud >= (int)sizeof(registro))
    {
        return FR_INT_ERR;
    }

    res = f_open(
        &archivo,
        "DATOS.CSV",
        FA_OPEN_ALWAYS | FA_WRITE
    );

    if (res != FR_OK)
    {
        return res;
    }

    res = f_lseek(&archivo, f_size(&archivo));

    if (res == FR_OK)
    {
        res = f_write(
            &archivo,
            registro,
            (UINT)longitud,
            &escritos
        );
    }

    if (res == FR_OK && escritos != (UINT)longitud)
    {
        res = FR_DISK_ERR;
    }

    cierre = f_close(&archivo);

    if (res == FR_OK)
    {
        res = cierre;
    }

    return res;
}

static FRESULT SD_LeerArchivo(void)
{
    FIL archivo;
    FRESULT resultado;
    FRESULT cierre;
    UINT bytesLeidos;

    char buffer[128];

    // Abrir archivo existente para lectura
    resultado = f_open(&archivo, "DATOS.CSV", FA_READ);

    if (resultado != FR_OK)
    {
        return resultado;
    }

    // Leer el archivo por bloques
    do
    {
        resultado = f_read(
            &archivo,
            buffer,
            sizeof(buffer) - 1,
            &bytesLeidos
        );

        if (resultado != FR_OK)
        {
            break;
        }

        // Agregar terminador de cadena
        buffer[bytesLeidos] = '\0';

        // Enviar contenido por UART
        if (bytesLeidos > 0)
        {
				if (HAL_UART_Transmit(
					    &huart5,
					    (uint8_t *)buffer,
					    (uint16_t)bytesLeidos,
					    1000
            ) != HAL_OK)
            {
                resultado = FR_INT_ERR;
                break;
            }
        }

    } while (bytesLeidos > 0);

    // Cerrar archivo
    cierre = f_close(&archivo);

    if (resultado == FR_OK)
    {
        resultado = cierre;
    }

    return resultado;
}

static void UART_Enviar(const char *texto)
{
    HAL_UART_Transmit(
        &huart5,
        (uint8_t *)texto,
        (uint16_t)strlen(texto),
        1000
    );
}

static void ProcesarComando(const char *comando)
{
    char respuesta[64];

    if (strcmp(comando, "LEER") == 0)
    {
        if (BH1750_Leer() == HAL_OK)
        {
            ultimaMedicion = (uint16_t)Mluz;
            medicionValida = 1;

            LuxMeter_Actualizar(ultimaMedicion);

            snprintf(
                respuesta,
                sizeof(respuesta),
                "MEDICION:%u lux\r\n",
                (unsigned int)ultimaMedicion
            );

            UART_Enviar(respuesta);
        }
        else
        {
            UART_Enviar("ERROR_SENSOR\r\n");
        }
    }
    else if (strcmp(comando, "GUARDAR") == 0)
    {
        if (!medicionValida)
        {
            UART_Enviar("ERROR_SIN_MEDICION\r\n");
            return;
        }

        FRESULT res = SD_GuardarMedicion();

        if (res == FR_OK)
        {
            UART_Enviar("GUARDADO_OK\r\n");
        }
        else
        {
            snprintf(
                respuesta,
                sizeof(respuesta),
                "ERROR_SD:%d\r\n",
                (int)res
            );

            UART_Enviar(respuesta);
        }
    }
    else if (strcmp(comando, "ESTADO") == 0)
    {
        if (medicionValida)
        {
            UART_Enviar("MEDICION_DISPONIBLE\r\n");
        }
        else
        {
            UART_Enviar("SIN_MEDICION\r\n");
        }
    }
    else if (strcmp(comando, "VER") == 0)
    {
        FRESULT res = SD_LeerArchivo();

        if (res != FR_OK)
        {
            snprintf(
                respuesta,
                sizeof(respuesta),
                "ERROR_LECTURA_SD:%d\r\n",
                (int)res
            );

            UART_Enviar(respuesta);
        }
    }
    else
    {
        UART_Enviar("COMANDO_DESCONOCIDO\r\n");
    }
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
  MX_SPI1_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_FATFS_Init();
  MX_USART2_UART_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */

  /* PRUEBA DE UART */


	LCD_Init();

	LCD_Clear(0x00);

	LuxMeter_DibujarBase();
	LuxMeter_Actualizar(0);

	uint8_t sensorListo = (BH1750_Iniciar() == HAL_OK);

	/* Montar tarjeta SD */
	resultadoSD = f_mount(&USERFatFS, USERPath, 1);

	if (resultadoSD == FR_OK)
	{
	    UART_Enviar("SD MONTADA CORRECTAMENTE\r\n");
	}
	else
	{
	    UART_Enviar("ERROR AL MONTAR SD\r\n");
	}
	if (resultadoSD == FR_OK)
	{
	    LCD_Print("SD OK", 220, 224, 1,
	              COLOR_VERDE, COLOR_PANEL);
	}
	else
	{
	    LCD_Print("SD ERROR", 220, 224, 1,
	              COLOR_ROJO, COLOR_PANEL);
	}

	UART_Enviar("NUCLEO 2 LISTA\r\n");
	UART_Enviar("COMANDOS: LEER, GUARDAR, ESTADO, VER\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	//uint16_t luxSimulado = 0;


	while (1)
	{
	    uint8_t caracter;

	    // Esperar un caracter desde Hercules por USART2
	    if (HAL_UART_Receive(&huart5, &caracter, 1, 20) == HAL_OK)
	    {
	        // Detectar fin del comando
	        if (caracter == '\r' || caracter == '\n')
	        {
	            if (indiceUART > 0)
	            {
	                // Terminar la cadena de texto
	                comandoUART[indiceUART] = '\0';

	                // Ejecutar LEER, GUARDAR, ESTADO o VER
	                ProcesarComando(comandoUART);

	                // Preparar para el siguiente comando
	                indiceUART = 0;
	            }
	        }
	        else if (indiceUART < sizeof(comandoUART) - 1)
	        {
	            // Guardar el caracter recibido
	            comandoUART[indiceUART++] = (char)caracter;
	        }
	        else
	        {
	            // El comando supera el tamaño permitido
	            indiceUART = 0;
	            UART_Enviar("ERROR_COMANDO_LARGO\r\n");
	        }
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
  RCC_OscInitStruct.PLL.PLLN = 80;
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin|LCD_D1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : LCD_RST_Pin LCD_D1_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RD_Pin LCD_WR_Pin LCD_RS_Pin LCD_D7_Pin
                           LCD_D0_Pin LCD_D2_Pin */
  GPIO_InitStruct.Pin = LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_D6_Pin LCD_D3_Pin LCD_D5_Pin
                           LCD_D4_Pin SD_SS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin|SD_SS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
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
	while (1) {
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
