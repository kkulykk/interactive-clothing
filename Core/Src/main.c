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
volatile int pressed = 0; // Ініціалізується нулем по замовчуванню, але так гарніше
volatile int button_is_pressed = 0;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{


 if( GPIO_Pin == GPIO_PIN_8)
 {
  static uint32_t last_change_tick;
  if( HAL_GetTick() - last_change_tick < 50 )
  {
   return;
  }
  last_change_tick = HAL_GetTick();
  if(button_is_pressed)
  {
   button_is_pressed = 0;
   ++pressed;
	  if (light == -1) {
		light = 0;
	  } else {
		light = -1;
	  }
  }else
  {
   button_is_pressed = 1;
  }
 }

 if( GPIO_Pin == GPIO_PIN_9)
 {
  static uint32_t last_change_tick;
  if( HAL_GetTick() - last_change_tick < 50 )
  {
   return;
  }
  last_change_tick = HAL_GetTick();
  if(button_is_pressed)
  {
   button_is_pressed = 0;
   ++pressed;
	  if (light == 1) {
		light = 0;
	  } else {
		light = 1;
	  }
  }else
  {
   button_is_pressed = 1;
  }
 }

 if( GPIO_Pin == GPIO_PIN_10)
 {
  static uint32_t last_change_tick;
  if( HAL_GetTick() - last_change_tick < 50 )
  {
   return;
  }
  last_change_tick = HAL_GetTick();
  if(button_is_pressed)
  {
   button_is_pressed = 0;
   ++pressed;
	  if (light == 2) {
		light = 0;
	  } else {
		light = 2;
	  }
  }else
  {
   button_is_pressed = 1;
  }
 }


}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
LCD5110_display lcd1;



#define MAX_LED 26 // max LEDs that we have in a cascade
#define USE_BRIGHTNESS 0


uint8_t LED_Data[MAX_LED][4];  // matrix of 4 columns, number of rows = number of LEDs we have
uint8_t LED_Mod[MAX_LED][4]; // for brightness


int datasentflag=0;  // to make sure that the dma does not send another data while the first data is still transmitted


void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)  // this callback is called when data transmission is finished
{
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);  // stop dma, when the transmission is finished
	datasentflag = 1;
}

void Set_LED (int LEDnum, int Red, int Green, int Blue)
{
	LED_Data[LEDnum][0] = LEDnum;
	LED_Data[LEDnum][1] = Green;  // store green first as ws2821b requires this order (g,r,b)
	LED_Data[LEDnum][2] = Red;
	LED_Data[LEDnum][3] = Blue;
}

#define PI 3.14159265359

void Set_Brightness (int brightness)  // 0-45
{
#if USE_BRIGHTNESS

	if (brightness > 45) brightness = 45;
	for (int i=0; i<MAX_LED; i++)
	{
		LED_Mod[i][0] = LED_Data[i][0];
		for (int j=1; j<4; j++)
		{
			float angle = 90-brightness;  // in degrees
			angle = angle*PI / 180;  // in rad
			LED_Mod[i][j] = (LED_Data[i][j])/(tan(angle));
		}
	}

#endif

}


uint16_t pwmData[(24*MAX_LED)+50]; // store 24 bits for each led + 50 values for reset code

void WS2812_Send (void)
{
	uint32_t indx=0;
	uint32_t color;  //32 bit variable to store 24 bits of color


	for (int i= 0; i<MAX_LED; i++)  // iterate through all of the LEDs
	{

#if USE_BRIGHTNESS
		color = ((LED_Mod[i][1]<<16) | (LED_Mod[i][2]<<8) | (LED_Mod[i][3]));
#else
		color = ((LED_Data[i][1]<<16) | (LED_Data[i][2]<<8) | (LED_Data[i][3]));
#endif
		for (int i=23; i>=0; i--) // iterate through the 24 bits which specify the color
		{
			if (color&(1<<i))
			{
				pwmData[indx] = 57; // if the bit is 1, the duty cycle is 64%
			}

			else pwmData[indx] = 28;  // if the bit is 0, the duty cycle is 32%

			indx++;
		}

	}

	for (int i=0; i<50; i++)  // store values to keep the pulse low for 50+ us, reset code
	{
		pwmData[indx] = 0;
		indx++;
	}
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwmData, indx);  // send the data to the dma
	while (!datasentflag){};  // this flag will be set when the data transmission is finished, dma is stopped and now we can send another data
	datasentflag = 0;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	  void attention_signal() {
			  for (int i = 0; i < 30; i++) {
				  Set_LED(i, 139, 0, 0);
				  Set_Brightness(45);
			  }

			  WS2812_Send();
			  HAL_Delay(700);

			  for (int i = 0; i < 30; i++) {
				  Set_LED(i, 0, 0, 0);
			  }

			  WS2812_Send();
			  HAL_Delay(550);

		  };

	  void turn_signal(int direction) {
		  int mid = MAX_LED / 2;

		  if (direction == -1){
	 		    for (int i = mid; i >= 0; i--) {
					Set_LED(i, 255, 69, 0);
					Set_Brightness(45);
					WS2812_Send();
					HAL_Delay(30);
			    }
		  }

		  if (direction == 1){
	 		    for (int i = mid; i < MAX_LED; i++) {
					Set_LED(i, 255, 69, 0);
					Set_Brightness(45);
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


//	  void turn_signal(int num, int direction) {
//
//		  int counter = 0;
//		  int led_num = direction == -1 ? num: 0;
//
//		  while (counter != num) {
//			Set_LED(led_num, 255, 30, 0);
//			Set_Brightness(45);
//			WS2812_Send();
//			HAL_Delay(30);
//			led_num += direction;
//			counter++;
//		};
//
//		 for (int i = 0; i < 30; i++) {
//		 	Set_LED(i, 0, 0, 0);
//		 }
//
//		 HAL_Delay(80);
//		 WS2812_Send();
//	  };





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

  LCD5110_print("Hello world!\n", BLACK, &lcd1);

  if(BSP_ACCELERO_Init() != HAL_OK)
  {
    /* Initialization Error */
	  LCD5110_print("Error initializing HAL.", BLACK, &lcd1);
    while(1){}
  }

  if(BSP_GYRO_Init() != HAL_OK){
	  /* Initialization Error */
	  	  LCD5110_print("Error initializing HAL.", BLACK, &lcd1);
	      while(1){}
  }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  int16_t buffer[3] = {0};
    float data[3] = {0};

   int fall_down = 0;


  while (1)
  {
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

	 if (light == 0){
			 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
			 HAL_Delay(200);
			 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
			 attention_signal();
		 }

	 if (light == 1) {
		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 1);
		 HAL_Delay(200);
		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
//		 warning_signal();
		 turn_signal(1);
	 }

	 if (light == -1) {
		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 1);
		 HAL_Delay(200);
		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 0);
		 turn_signal(-1);
	 }

	 if (light == 2) {
		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 1);
		 HAL_Delay(200);
		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);
		 warning_signal();
	 }

	   void show_accelerometer(){
//		   while(1) {
		   BSP_ACCELERO_GetXYZ(buffer);
		   double x = (buffer[0]/16)/1000.0;
		   double y = (double)(buffer[1]/16)/1000.0;
		   double z = (double)(buffer[2]/16)/1000.0;
		   double length = pow((x*x + y*y + z*z), 0.5);

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
		  LCD5110_print("Accelerometer\n", BLACK, &lcd1);
		  LCD5110_printf(&lcd1, BLACK, "X: %g \n Y: %g \n Z: %g \n", x, y, z);
		  LCD5110_printf(&lcd1, BLACK, "length: %g \n", length);
		  HAL_Delay(100);
		  LCD5110_clear_scr(&lcd1);
//		  if ( HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET )
//		     {break;
//		     	while( HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET )
//		     	{}
//		     	HAL_Delay(50);
//		     }
//	   }
	   };

//	   void show_gyroscope() {
//		   while(1){
//		   BSP_GYRO_GetXYZ(data);
//		   double x = data[0]/1000;
//		   double y = data[1]/1000;
//		   double z = data[2]/1000;
//		   double length = pow((x*x + y*y + z*z), 0.5);
//		   LCD5110_print("Gyroscope\n", BLACK, &lcd1);
//		   LCD5110_printf(&lcd1, BLACK, "X: %g \n Y: %g \n Z: %g \n", x, y, z);
//		   LCD5110_printf(&lcd1, BLACK, "length: %g \n", length);
//		   HAL_Delay(100);
//		   LCD5110_clear_scr(&lcd1);
//		   if ( HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET )
//		   		     {break;
//		   		  	while( HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET )
//		   		  	{}
//		   		  	HAL_Delay(50);}
//	   }
//	   };


	   	   show_accelerometer();
//	   	   show_gyroscope();
	   	   while( HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET )
	   	   		     	{}
	   	   		     	HAL_Delay(50);



	   	if (fall_down == 1) {
		   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 1);
		   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
		   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 1);
		   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 1);
	   	}

	   	if (fall_down == 0) {
		   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
		   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
		   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 0);
		   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);
	   	}

//		 if ( HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET )
//		 {
//			 			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 1);
//			 			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
//			 			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 1);
//			 			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 1);
//		 } else {
//			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
//			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
//			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 0);
//			   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);
//
//		 }






//	   GPIO_PinState btnState = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_8);
//	   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, btnState);

//	  attention_signal();
//	  turn_signal(-1);
//	  warning_signal();

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
