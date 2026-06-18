/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Left_Foward_Pin GPIO_PIN_0
#define Left_Foward_GPIO_Port GPIOC
#define Left_Reverse_Pin GPIO_PIN_1
#define Left_Reverse_GPIO_Port GPIOC
#define Right_Forward_Pin GPIO_PIN_2
#define Right_Forward_GPIO_Port GPIOC
#define Right_Reverse_Pin GPIO_PIN_3
#define Right_Reverse_GPIO_Port GPIOC
#define Left_motor_PWM_Pin GPIO_PIN_0
#define Left_motor_PWM_GPIO_Port GPIOA
#define Right_motor_PWM_Pin GPIO_PIN_1
#define Right_motor_PWM_GPIO_Port GPIOA
#define Trig_L_Pin GPIO_PIN_8
#define Trig_L_GPIO_Port GPIOA
#define Trig_C_Pin GPIO_PIN_9
#define Trig_C_GPIO_Port GPIOA
#define Trig_R_Pin GPIO_PIN_10
#define Trig_R_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
