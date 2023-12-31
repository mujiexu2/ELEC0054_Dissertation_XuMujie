/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "i2c.h"
#include "rtc.h"
#include "spi.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "icm20948.h"
#include "time.h"
#include "usbd_cdc_if.h"
#include <string.h>
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
//initialize variables for storing imu data
axises my_gyro;
axises my_accel;
axises my_mag;

icm_20948_data imu_data;

//char usbd_ch;
//int Flag = 0;

//char buffer1[] = "You sent 1, get 1 back\r\n";
//char buffer2[] = "You sent 2, get 2 back\r\n";
//char buffer3[] = "You sent 3, get 3 back\r\n";

char received_data[1000];

char time[30];
char date[30];

RTC_TimeTypeDef sTime = {0};
RTC_DateTypeDef sDate = {0};

NMEA_Result NMEA_result;

//organized data for future sending, including the time data and sensor data
typedef struct {
    time_data time_info;
    icm_20948_data sensor_data;
} combined_data;


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
	//initialize time data and sensor data
	icm_20948_data imu_data;
	time_data time_result;
	combined_data dataToSend;7
	//get start time for getting the elapsed time later
	uint32_t startTime = HAL_GetTick();
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
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  //initialize ICM gyroscope, accelerometer and magnetometer peripherals and configuration
  icm20948_init();
  ak09916_init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	// This segment fetches sensor data, combines it with time information,
	// formats it into a specific string format, and sends it over USB.
	  dataToSend.time_info = read_time(startTime); // Assume you already have the read_time function
	  dataToSend.sensor_data = read_all_data(); // Assume you have modified the read_all_data function as previously indicated

	  char buffer[512]; // suppose 512 bytes is big enough
	  // Creating a formatted string from the combined time and sensor data
	  // part to insert special character that enable future data splitting
	  sprintf(buffer,
			  "#%u&" //unix timestamp
			  "%04d-%02d-%02d %02d:%02d:%02d&" // utc_timestamp
			  "%04d-%02d-%02d %02d:%02d:%02d&" //
			  "%02u:%02u&"//
			  "x_accel = %f/y_accel = %f/z_accel = %f&"
			  "x_gyro = %f/y_gyro = %f/z_gyro =  %f&"
			  "x_mag = %f/y_mag = %f/z_mag = %f&\r\n",
			  dataToSend.time_info.unix_timestamp,

			  dataToSend.time_info.utc_time.year,
			  dataToSend.time_info.utc_time.month,
			  dataToSend.time_info.utc_time.date,
			  dataToSend.time_info.utc_time.hour,
			  dataToSend.time_info.utc_time.min,
			  dataToSend.time_info.utc_time.sec,

			  dataToSend.time_info.uk_time.year,
			  dataToSend.time_info.uk_time.month,
			  dataToSend.time_info.uk_time.date,
			  dataToSend.time_info.uk_time.hour,
			  dataToSend.time_info.uk_time.min,
			  dataToSend.time_info.uk_time.sec,

			  dataToSend.time_info.elapsed_minutes,
			  dataToSend.time_info.elapsed_seconds,

			  dataToSend.sensor_data.x_accel,
			  dataToSend.sensor_data.y_accel,
			  dataToSend.sensor_data.z_accel,

			  dataToSend.sensor_data.x_gyro,
			  dataToSend.sensor_data.y_gyro,
			  dataToSend.sensor_data.z_gyro,

			  dataToSend.sensor_data.x_magnet,
			  dataToSend.sensor_data.y_magnet,
			  dataToSend.sensor_data.z_magnet

	  );
	  //transmit to the USB VCP
	  CDC_Transmit_FS(buffer, strlen(buffer));



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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

//printf function
/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len)
{
	int DataIdx;

	for (DataIdx = 0; DataIdx < len; DataIdx++)
	{
		ITM_SendChar(*ptr++);
	}
	return len;
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
