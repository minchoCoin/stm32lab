/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define MPU9250_ADDR 0x68<<1     // AD0 핀이 GND에 연결된 경우
#define WHO_AM_I     0x75
#define CONFIG       0x1A
#define GYRO_CONFIG  0x1B
#define ACCEL_CONFIG 0x1C
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H  0x43
#define AK8963_ADDR  0x0C<<1
#define AK8963_ST1   0x02
#define AK8963_ST2   0x09
#define AK8963_XOUT_L 0x03
#define AK8963_CNTL1 0x0A
#define PWR_MGMT_1 0x6B

typedef struct {
    float accel[3];    // x, y, z 가속도 값
    float gyro[3];     // x, y, z 자이로 값
    float mag[3];      // x, y, z 자기장 값
} MPU9250_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
MPU9250_t mpu9250_data;
HAL_StatusTypeDef status;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void send_errmsg(char* msg){
	HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 100);
}
uint8_t MPU9250_Init(void)
{
    uint8_t check, data;

    // WHO_AM_I 레지스터 확인
    HAL_I2C_Mem_Read(&hi2c1, MPU9250_ADDR, WHO_AM_I, 1, &check, sizeof(check), 100);
    if(check!=0x71)  // MPU9250 ID
        return 1;      // 에러 반환

    // 전원 관리 레지스터 초기화
    data = 0x00;       // 내부 8MHz 오실레이터 사용
    status=HAL_I2C_Mem_Write(&hi2c1, MPU9250_ADDR, PWR_MGMT_1, 1, &data, sizeof(data), 100);
    if(status!=HAL_OK){
    	send_errmsg("error on PWR_MGMT_1\r\n");
    	Error_Handler();

    }


    // 자이로스코프 설정 (±250dps)
    data = 0x00;
    status=HAL_I2C_Mem_Write(&hi2c1, MPU9250_ADDR, GYRO_CONFIG, 1, &data, sizeof(data), 100);
    if(status!=HAL_OK){
    	send_errmsg("error on GYRO_CONFIG\r\n");
    	Error_Handler();

    }


    // 가속도계 설정 (±2g)
    data = 0x00;
    status=HAL_I2C_Mem_Write(&hi2c1, MPU9250_ADDR, ACCEL_CONFIG, 1, &data, sizeof(data), 100);
    if(status!=HAL_OK){
    	send_errmsg("error on ACCEL_CONFIG\r\n");
    	Error_Handler();
    }


    return 0;
}
void AK8963_Init(){
	uint8_t data;
	// I2C 마스터 모드 비활성화
	//data = 0x00;
	//HAL_I2C_Mem_Write(&hi2c1, MPU9250_ADDR, 0x6A, 1, &data, 1, 100);

	// I2C 패스스루 활성화
	data = 0x02;
	status=HAL_I2C_Mem_Write(&hi2c1, MPU9250_ADDR, 0x37, 1, &data, 1, 100);
	if(status!=HAL_OK){
	    	send_errmsg("error on I2C paththrough\r\n");
	    	Error_Handler();

	    }
	HAL_Delay(100);
	data = 0x00;
	    HAL_I2C_Mem_Write(&hi2c1, AK8963_ADDR, AK8963_CNTL1, 1, &data, 1, 100);
	    if(status!=HAL_OK){
	    	    	send_errmsg("error on I2C init\r\n");
	    	    	Error_Handler();
	    }
	    HAL_Delay(10);

	    // 16비트 출력, continuous 설정
	    data = 0x16;
	    status=HAL_I2C_Mem_Write(&hi2c1, AK8963_ADDR, AK8963_CNTL1, 1, &data, 1, 100);
	    if(status!=HAL_OK){
	    	    	    	send_errmsg("error on ak8963_16bitoutputconfig\r\n");
	    	    	    	Error_Handler();

	    	    	    }
	    HAL_Delay(10);
}
void MPU9250_ReadAccel(void)
{
    uint8_t data[6];
    int16_t raw_data[3];

    status=HAL_I2C_Mem_Read(&hi2c1, MPU9250_ADDR, ACCEL_XOUT_H, 1, data, sizeof(data), 100);
    if(status!=HAL_OK){
    	send_errmsg("error on read accel\r\n");
    	    	Error_Handler();
    }

    raw_data[0] = (data[0] << 8) | data[1];    // X축
    raw_data[1] = (data[2] << 8) | data[3];    // Y축
    raw_data[2] = (data[4] << 8) | data[5];    // Z축

    // ±2g 범위에서 변환 (16384 LSB/g)
    mpu9250_data.accel[0] = raw_data[0] / 16384.0f;
    mpu9250_data.accel[1] = raw_data[1] / 16384.0f;
    mpu9250_data.accel[2] = raw_data[2] / 16384.0f;
}

/* 자이로 데이터 읽기 */
void MPU9250_ReadGyro(void)
{
    uint8_t data[6];
    int16_t raw_data[3];

    status=HAL_I2C_Mem_Read(&hi2c1, MPU9250_ADDR, GYRO_XOUT_H, 1, data, sizeof(data), 100);
    if(status!=HAL_OK){
    	send_errmsg("error on read gyro\r\n");
    	Error_Handler();
    }


    raw_data[0] = (data[0] << 8) | data[1];
    raw_data[1] = (data[2] << 8) | data[3];
    raw_data[2] = (data[4] << 8) | data[5];

    // ±250dps 범위에서 변환 (131 LSB/dps)
    mpu9250_data.gyro[0] = raw_data[0] / 131.0f;
    mpu9250_data.gyro[1] = raw_data[1] / 131.0f;
    mpu9250_data.gyro[2] = raw_data[2] / 131.0f;
}

/* 자기장 데이터 읽기 */
void MPU9250_ReadMag(void)
{
    uint8_t data[6];
    int16_t raw_data[3];



        // 자기장 데이터 읽기
        status=HAL_I2C_Mem_Read(&hi2c1, AK8963_ADDR, AK8963_XOUT_L, 1, data, sizeof(data), 100);
        if(status!=HAL_OK){
            	send_errmsg("error on read ak8963_data\r\n");
            	Error_Handler();
            }
        raw_data[0] = (data[1] << 8) | data[0];    // X축
        raw_data[1] = (data[3] << 8) | data[2];    // Y축
        raw_data[2] = (data[5] << 8) | data[4];    // Z축

        // 변환 (0.15μT/LSB)
        mpu9250_data.mag[0] = raw_data[0] * 0.15f;
        mpu9250_data.mag[1] = raw_data[1] * 0.15f;
        mpu9250_data.mag[2] = raw_data[2] * 0.15f;

        //make sure read ST2 after read mag data
        status=HAL_I2C_Mem_Read(&hi2c1, AK8963_ADDR, AK8963_ST2, 1, &(data[0]), sizeof(data[0]), 100);
        if(status!=HAL_OK){
                    	send_errmsg("error on read ak8963_st2\r\n");
                    	Error_Handler();
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
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  if(MPU9250_Init() != 0)
      {
	  char* errmsg = "error occurred on MPU9250 initialization\r\n";
	  	  HAL_UART_Transmit(&huart1, (uint8_t *)errmsg, strlen(errmsg), 100);
          Error_Handler();  // MPU9250 초기화 실패
      }
  AK8963_Init();
  while (1)
  {

    /* USER CODE END WHILE */

	  MPU9250_ReadAccel();

	  MPU9250_ReadGyro();

	  MPU9250_ReadMag();
	  char strbuf[500];
	          // 예: UART를 통해 데이터 전송
	          int len = sprintf(strbuf,"Accel: %.2f %.2f %.2f, Gyro: %.2f %.2f %.2f, Mag: %.2f, %.2f, %.2f\r\n",
	                 mpu9250_data.accel[0],
	                 mpu9250_data.accel[1],
	                 mpu9250_data.accel[2],
					 mpu9250_data.gyro[0],
					 mpu9250_data.gyro[1],
					 mpu9250_data.gyro[2],
					 mpu9250_data.mag[0],
					 mpu9250_data.mag[1],
					 mpu9250_data.mag[2]);
	          HAL_UART_Transmit(&huart1, (uint8_t *)strbuf, len, 100);
	          HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_9);
	          HAL_Delay(500);  // 100ms 간격으로 데이터 읽기
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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
  while (1)
  {
	  char* errmsg = "Error occurred\r\n";
	  HAL_UART_Transmit(&huart1, (uint8_t *)errmsg, strlen(errmsg), 100);
	  HAL_Delay(1000);
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
