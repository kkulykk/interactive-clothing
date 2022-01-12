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
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usb_host.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "math.h"
#include <stdbool.h>
#include "lcd5110.h"
#include "stm32f411e_discovery_accelerometer.h"
#include "stm32f411e_discovery_gyroscope.h"
#include "stm32f411e_discovery.h"

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
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

volatile int fall_down = 0;
volatile int light = 0;
volatile int pressed = 0;
volatile int button_is_pressed = 0;
volatile int loop_counter = 0;


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

 if (GPIO_Pin == GPIO_PIN_8) {

  static uint32_t last_change_tick;
  if (HAL_GetTick() - last_change_tick < 50) {
	  return;
  }

  last_change_tick = HAL_GetTick();

  if (button_is_pressed) {
	  button_is_pressed = 0;
	  ++pressed;
	  if (light == -1) {
		  light = 0;
	  } else {
		  light = -1;
	  }
  } else {
	  button_is_pressed = 1;
  }
 }


 if (GPIO_Pin == GPIO_PIN_9) {

	 static uint32_t last_change_tick;

	 if( HAL_GetTick() - last_change_tick < 50) {
		 return;
	 }
	 last_change_tick = HAL_GetTick();

	 if (button_is_pressed)
	 {
	   button_is_pressed = 0;
	   ++pressed;
	   if (light == 1) {
		   light = 0;
	   } else {
		   light = 1;
	   }
	 } else {
		 button_is_pressed = 1;
	 }
 }

 if (GPIO_Pin == GPIO_PIN_10) {
	 static uint32_t last_change_tick;

	 if (HAL_GetTick() - last_change_tick < 50) {
		 return;
	 }
	 last_change_tick = HAL_GetTick();

	 if (button_is_pressed) {
		 button_is_pressed = 0;
		 ++pressed;

		 if (light == 2) {
			 light = 0;
		 } else {
			 light = 2;
		 }
	 } else {
		 button_is_pressed = 1;
	 }
 }
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define MAX_LED 26
#define USE_BRIGHTNESS 0
#define PI 3.14159265359

#define MPU6050_ADDR 0xD0
#define SMPLRT_DIV_REG 0x19
#define GYRO_CONFIG_REG 0x1B
#define ACCEL_CONFIG_REG 0x1C
#define ACCEL_XOUT_H_REG 0x3B
#define TEMP_OUT_H_REG 0x41
#define GYRO_XOUT_H_REG 0x43
#define PWR_MGMT_1_REG 0x6B
#define WHO_AM_I_REG 0x75

LCD5110_display lcd1;

volatile uint32_t tim10_overflows = 0;
volatile float Ax, Ay, Az, Gx, Gy, Gz;
int datasentflag=0;

uint8_t LED_Data[MAX_LED][4];
uint8_t LED_Mod[MAX_LED][4];

int16_t Accel_X_RAW = 0;
int16_t Accel_Y_RAW = 0;
int16_t Accel_Z_RAW = 0;

int16_t Gyro_X_RAW = 0;
int16_t Gyro_Y_RAW = 0;
int16_t Gyro_Z_RAW = 0;



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {

  if (htim -> Instance == TIM10) {
	  MPU6050_Read_Accel();
	  ++tim10_overflows;
  }

}

static inline void TIM10_reinit() {

	HAL_TIM_Base_Stop(&htim10);
	__HAL_TIM_SET_COUNTER( &htim10, 0 );
	tim10_overflows = 0;
	HAL_TIM_Base_Start_IT(&htim10);

}

static inline uint32_t get_tim10_us() {

	__HAL_TIM_DISABLE_IT(&htim10, TIM_IT_UPDATE);
	uint32_t res = tim10_overflows * 10000 + __HAL_TIM_GET_COUNTER(&htim10);
	__HAL_TIM_ENABLE_IT(&htim10, TIM_IT_UPDATE);
	return res;

}

static inline void udelay_TIM10(uint32_t useconds) {

	uint32_t before = get_tim10_us();

	while( get_tim10_us() < before+useconds){}

}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {  // callback when dma finished data transfering

	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);  // stop dma, when data transfering has finished
	datasentflag = 1;

}

void Set_LED (int LEDnum, int Red, int Green, int Blue) {

	LED_Data[LEDnum][0] = LEDnum;
	LED_Data[LEDnum][1] = Green;
	LED_Data[LEDnum][2] = Red;
	LED_Data[LEDnum][3] = Blue;

}


uint16_t pwmData[(24*MAX_LED)+50]; // 24 bits leds + 50 eset code

void WS2812_Send (void) {

	uint32_t indx=0;
	uint32_t color;

	for (int i = 0; i < MAX_LED; i++) {

		#if USE_BRIGHTNESS
		color = ((LED_Mod[i][1]<<16) | (LED_Mod[i][2]<<8) | (LED_Mod[i][3]));
		#else
		color = ((LED_Data[i][1]<<16) | (LED_Data[i][2]<<8) | (LED_Data[i][3]));
		#endif

		for (int i = 23; i >= 0; i--) {

			if (color&(1 << i)) {
				pwmData[indx] = 57; // duty cycle is 64% from datasheet
			}

			else pwmData[indx] = 28;  // if the bit is 0, the duty cycle is 32%
			indx++;
		}
	}

	for (int i = 0; i < 50; i++) { // reset code
		pwmData[indx] = 0;
		indx++;
	}

	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwmData, indx);  // send to dma

	while (!datasentflag){};  // set flag when data has been transmitted
	datasentflag = 0;

}


void MPU6050_Init (void) {

	uint8_t check;
	uint8_t Data;

	HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR,WHO_AM_I_REG,1, &check, 1, 1000);

	if (check == 104) { // 0x68 will be returned by the sensor if everything goes well
    // power management register 0X6B we should write all 0's to wake the sensor up
		Data = 0;
		HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, 1,&Data, 1, 1000);

    // Set DATA RATE of 1KHz by writing SMPLRT_DIV register
		Data = 0x07;
		HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, SMPLRT_DIV_REG, 1, &Data, 1, 1000);

    // Set accelerometer configuration in ACCEL_CONFIG Register
    // XA_ST=0,YA_ST=0,ZA_ST=0, FS_SEL=0 -> � 2g
		Data = 0x00;
    	HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, ACCEL_CONFIG_REG, 1, &Data, 1, 1000);

    // Set Gyroscopic configuration in GYRO_CONFIG Register
    // XG_ST=0,YG_ST=0,ZG_ST=0, FS_SEL=0 -> � 250 �/s
    	Data = 0x00;
    	HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, GYRO_CONFIG_REG, 1, &Data, 1, 1000);
	}

}

void MPU6050_Read_Accel (void) {

	uint8_t Rec_Data[6];

	// Read 6 BYTES of data starting from ACCEL_XOUT_H register

	HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG, 1, Rec_Data, 6, 1000);

	Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data [1]);
	Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data [3]);
	Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data [5]);

	/*** convert the RAW values into acceleration in 'g'
       we have to divide according to the Full scale value set in FS_SEL
       I have configured FS_SEL = 0. So I am dividing by 16384.0
       for more details check ACCEL_CONFIG Register              ****/

	Ax = Accel_X_RAW/16384.0;
	Ay = Accel_Y_RAW/16384.0;
	Az = Accel_Z_RAW/16384.0;

	loop_counter++;

}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

int main(void) {
  /* USER CODE BEGIN 1 */
	  void attention_signal() {

		  for (int i = 0; i < 30; i++) {
			  Set_LED(i, 139, 0, 0);
		  }

		  WS2812_Send();
		  HAL_Delay(700);

		  for (int i = 0; i < 30; i++) {
			  Set_LED(i, 0, 0, 0);
		  }

		  WS2812_Send();
		  HAL_Delay(550);

	  };

	  void turn_signal (int direction) {

		  int mid = MAX_LED / 2;

		  if (direction == -1) {
			  for (int i = mid; i >= 0; i--) {
				  Set_LED(i, 255, 69, 0);
				  WS2812_Send();
				  HAL_Delay(30);
			  }
		  }

		  if (direction == 1) {
			  for (int i = mid; i < MAX_LED; i++) {
				  Set_LED(i, 255, 69, 0);
				  WS2812_Send();
				  HAL_Delay(30);
			  }
		  }

		 for (int i = 0; i < MAX_LED; i++) {
			 Set_LED(i, 0, 0, 0);
		 }

		 HAL_Delay(120);
		 WS2812_Send();

	  };


	  void warning_signal() {

		  int mid = MAX_LED / 2;

		  for (int i = 0; i <= mid; i++) {
			  Set_LED(mid + i, 255, 69, 0);
			  Set_LED(mid - i, 255, 69, 0);
			  Set_Brightness(45);
			  WS2812_Send();
			  HAL_Delay(30);
		  }

		 for (int i = 0; i < MAX_LED; i++) {
		 	Set_LED(i, 0, 0, 0);
		 }

		 HAL_Delay(120);
		 WS2812_Send();

	  };


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
  MX_DMA_Init();
  MX_USB_HOST_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  MX_SPI2_Init();
  MX_TIM10_Init();
  /* USER CODE BEGIN 2 */


  lcd1.hw_conf.spi_handle = &hspi2;
  lcd1.hw_conf.spi_cs_pin =  LCD1_CS_Pin;
  lcd1.hw_conf.spi_cs_port = LCD1_CS_GPIO_Port;
  lcd1.hw_conf.rst_pin =  LCD1_RST_Pin;
  lcd1.hw_conf.rst_port = LCD1_RST_GPIO_Port;
  lcd1.hw_conf.dc_pin =  LCD1_DC_Pin;
  lcd1.hw_conf.dc_port = LCD1_DC_GPIO_Port;
  lcd1.def_scr = lcd5110_def_scr;
  LCD5110_init(&lcd1.hw_conf, LCD5110_NORMAL_MODE, 0x40, 2, 3);

  if (BSP_ACCELERO_Init() != HAL_OK) {
    /* Initialization Error */
	  LCD5110_print("Error initializing HAL.", BLACK, &lcd1);
	  while(1){}
  }

  if (BSP_GYRO_Init() != HAL_OK) {
	  /* Initialization Error */
	  LCD5110_print("Error initializing HAL.", BLACK, &lcd1);
	  while(1){}
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


  int16_t buffer[3] = {0};

  MPU6050_Init();
  TIM10_reinit();

  while (1)
  {
	  //Button change modes method
	//	 if ( HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_8) == GPIO_PIN_RESET )
	//	 {
	//	  if (light == 1) {
	//		light = 0;
	//	  } else {
	//		light = 1;
	//	  }
	//	HAL_Delay(50);
	//	while( HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_8) == GPIO_PIN_RESET )
	//	{}
	//	HAL_Delay(50);
	//	 }

	//	 if (light == 0){
	//			 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
	//			 HAL_Delay(200);
	//			 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
	//			 attention_signal();
	//		 }
	//
	//	 if (light == 1) {
	//		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 1);
	//		 HAL_Delay(200);
	//		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
	////		 warning_signal();
	//		 turn_signal(1);
	//	 }
	//
	//	 if (light == -1) {
	//		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 1);
	//		 HAL_Delay(200);
	//		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 0);
	//		 turn_signal(-1);
	//	 }
	//
	//	 if (light == 2) {
	//		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 1);
	//		 HAL_Delay(200);
	//		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);
	//		 warning_signal();
	//	 }


	  void show_accelerometer() {

		  BSP_ACCELERO_GetXYZ(buffer);
		  double x = (buffer[0]/16)/1000.0;
		  double y = (double)(buffer[1]/16)/1000.0;
		  double z = (double)(buffer[2]/16)/1000.0;

		  if (fabs(z) > 0.85 ||  fabs(x) > 0.8) {
			  fall_down = 1;
			  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 1);
			  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
			  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 1);
			  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 1);
		  }
//		   else {
//			   fall_down = 0;
//			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
//			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
//			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 0);
//			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);
//		   }
		  };

	  if (Az > 0.8) {
		  turn_signal(1);
	  } else if (Ax > 0.8) {
		  turn_signal(-1);
		} else {
			attention_signal();
		}

//	 MPU6050_Read_Accel();
	 LCD5110_printf(&lcd1, BLACK, "Ax=%f \n", Ax);
	 LCD5110_printf(&lcd1, BLACK, "Ay=%f \n", Ay);
	 LCD5110_printf(&lcd1, BLACK, "Az=%f \n", Az);
	 LCD5110_printf(&lcd1, BLACK, "Count=%i \n", loop_counter);

	 LCD5110_clear_scr(&lcd1);

    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
