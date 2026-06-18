/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
// ⭐️ 블루투스 수신 및 상태 변수
uint8_t rx_data;
char car_mode = 'M'; // 'M'anual (수동 모드), 'A'uto (자율주행 모드)
int current_speed = 50;  // 현재 설정된 최고 속도 (0~100%)

// ⭐️ 현재 수동 조작 목표값
int manual_throttle = 0;
int manual_steering = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
// ⭐️ 모터 제어 믹싱 함수 이름표 등록
void Motor_Drive(int throttle, int steering);
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
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  // 1. 모터 제어용 타이머(TIM2) PWM 출력 시작 (PA0, PA1)
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); 
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
  HAL_TIM_Base_Start(&htim2);

  // 2. 초음파 센서 거리 측정용 1us 스톱워치(TIM3) 시작!
  HAL_TIM_Base_Start(&htim3);

  // 3. ⭐️ 블루투스 UART 수신 인터럽트 가동 (1바이트씩 받기 시작!)
  HAL_UART_Receive_IT(&huart2, &rx_data, 1);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // FreeRTOS가 실행되면 메인 루프는 더 이상 돌지 않으므로 비워둡니다.
      // (자율주행 코드는 나중에 freertos.c의 Task 안에서 작성하게 됩니다!)
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

// =========================================================================
// ⭐️ 1. 블루투스 UART 인터럽트 수신 처리 (테슬라 오토파일럿 방식 적용!)
// =========================================================================
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {

        // 1. 받은 알파벳(명령어) 판별
    	switch(rx_data) {
    	            // 방향키 입력
    	            case 'F': car_mode = 'M'; manual_throttle = current_speed;  manual_steering = 0; break;
    	            case 'B': car_mode = 'M'; manual_throttle = -current_speed; manual_steering = 0; break;
    	            case 'L': car_mode = 'M'; manual_throttle = 0; manual_steering = -current_speed; break;
    	            case 'R': car_mode = 'M'; manual_throttle = 0; manual_steering = current_speed;  break;

    	            // 정지 (버튼에서 손을 뗄 때 전송되는 '0' 및 Square 버튼 'S')
    	            case 'S':
    	            case '0':
    	                car_mode = 'M'; manual_throttle = 0; manual_steering = 0; break;

    	            // 속도 증가 (Triangle 버튼)
    	            case 'T':
    	                current_speed += 10;
    	                if(current_speed > 100) current_speed = 100;
    	                if(car_mode == 'M' && manual_throttle > 0) manual_throttle = current_speed;
    	                else if(car_mode == 'M' && manual_throttle < 0) manual_throttle = -current_speed;
    	                break;

    	            // 속도 감소 (Cross 버튼)
    	            case 'X':
    	                current_speed -= 10;
    	                if(current_speed < 0) current_speed = 0;
    	                if(car_mode == 'M' && manual_throttle > 0) manual_throttle = current_speed;
    	                else if(car_mode == 'M' && manual_throttle < 0) manual_throttle = -current_speed;
    	                break;

    	            // 자율주행 모드 (Start 버튼)
    	            case 'A':
    	                car_mode = 'A';
    	                break;

    	            // 수동 정지 모드 (Pause 버튼)
    	            case 'P':
    	            case 'M':
    	                car_mode = 'M'; manual_throttle = 0; manual_steering = 0; break;
    	        }

        // 2. 수동 모드일 때만 모터를 즉각 돌려줍니다! (자율주행일 땐 무시)
        if (car_mode == 'M') {
            Motor_Drive(manual_throttle, manual_steering);
        }

        // 3. 다음 알파벳을 받기 위해 다시 인터럽트 대기
        HAL_UART_Receive_IT(&huart2, &rx_data, 1);
    }
}

// =========================================================================
// ⭐️ 2. 모터 드라이브 믹싱 & 출력 함수 (PA0, PA1 / PC0~PC3)
// =========================================================================
void Motor_Drive(int throttle, int steering)
{
    // 좌/우 바퀴 속도 믹싱 계산
    int left_speed = throttle + steering;
    int right_speed = throttle - steering;

    // 안전장치: 속도는 -100 ~ 100 을 넘을 수 없음
    if(left_speed > 100) left_speed = 100;
    if(left_speed < -100) left_speed = -100;
    if(right_speed > 100) right_speed = 100;
    if(right_speed < -100) right_speed = -100;

    // ---------------- 좌측 모터 방향 제어 (PC0, PC1) ----------------
    if(left_speed >= 0) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);   // Left Forward (PC0)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET); // Left Reverse (PC1)
    } else {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
        left_speed = -left_speed; // PWM 출력을 위해 절댓값(+)으로 변경
    }

    // ---------------- 우측 모터 방향 제어 (PC2, PC3) ----------------
    if(right_speed >= 0) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);   // Right Forward (PC2)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); // Right Reverse (PC3)
    } else {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
        right_speed = -right_speed; // PWM 출력을 위해 절댓값(+)으로 변경
    }

    // ---------------- PWM 속도 출력 (PA0, PA1) ----------------
    // ⚠️ 큐브MX에서 TIM2의 Counter Period (ARR) 값이 '100-1' 로 설정되어 있다고 가정합니다.
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, left_speed*10);  // Left PWM (PA0)
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, right_speed*10); // Right PWM (PA1)
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM11 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM11)
  {
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
