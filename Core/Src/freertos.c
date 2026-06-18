/* USER CODE BEGIN Header */

/**

  ******************************************************************************

  * File Name          : freertos.c

  ******************************************************************************

  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "tim.h"

extern void Motor_Drive(int throttle, int steering);

extern char car_mode;

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {

    STATE_STRAIGHT = 0,

    STATE_TURN_LEFT,

    STATE_TURN_RIGHT,

    STATE_REVERSE_AVOID

} DrivingState;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

uint32_t distance_L = 0;

uint32_t distance_C = 0;

uint32_t distance_R = 0;



// Input Capture 상태 관리 배열 (0: Left, 1: Center, 2: Right)

uint8_t ic_flag[3] = {0, 0, 0};

uint32_t ic_val1[3] = {0, 0, 0};

uint32_t ic_val2[3] = {0, 0, 0};



void delay_us(uint16_t time)

{

    uint32_t delay_count = time * (100 / 4);

    while(delay_count--) {

        __asm("nop");

    }

}

/* USER CODE END Variables */
/* Definitions for Motortask */
osThreadId_t MotortaskHandle;
const osThreadAttr_t Motortask_attributes = {
  .name = "Motortask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartMotorTask(void *argument);
void StartSensorTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Motortask */
  MotortaskHandle = osThreadNew(StartMotorTask, NULL, &Motortask_attributes);

  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartMotorTask */
/**
  * @brief  Function implementing the Motortask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartMotorTask */
void StartMotorTask(void *argument)
{
  /* USER CODE BEGIN StartMotorTask */

  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);

  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);

  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_3);



  static float previous_error = 0;

  static float filtered_L = 0, filtered_R = 0, filtered_C = 0;

  static float current_steering = 0;

  static DrivingState current_state = STATE_STRAIGHT;



  for(;;)

  {

      int raw_L = (distance_L == 0 || distance_L > 40) ? 40 : distance_L;

      int raw_R = (distance_R == 0 || distance_R > 40) ? 40 : distance_R;

      int raw_C = (distance_C == 0 || distance_C > 50) ? 50 : distance_C;



      filtered_L = (filtered_L * 0.7f) + (raw_L * 0.3f);

      filtered_R = (filtered_R * 0.7f) + (raw_R * 0.3f);

      filtered_C = (filtered_C * 0.7f) + (raw_C * 0.3f);



      if (car_mode != 'A') {

          osDelay(20);

          continue;

      }



      int throttle = 0;

      float target_steering = 0;



      // =========================================================

      // 🧠 두뇌 영역: 판단 로직

      // =========================================================

      if (current_state == STATE_REVERSE_AVOID) {

          if (filtered_L > 35.0f) {

              current_state = STATE_TURN_LEFT;

          }

          else if (filtered_R > 35.0f) {

              current_state = STATE_TURN_RIGHT;

          }

          else if (filtered_C > 20.0f && filtered_L > 10.0f && filtered_R > 10.0f) {

              current_state = STATE_STRAIGHT;

          }

      }

      else if (filtered_C <= 12.0f || filtered_L <= 7.0f || filtered_R <= 7.0f) {

          current_state = STATE_REVERSE_AVOID;

      }

      else if (current_state == STATE_TURN_LEFT || current_state == STATE_TURN_RIGHT) {

          if (filtered_C > 20.0f) {

              current_state = STATE_STRAIGHT;

          }

      }

      else if (filtered_C <= 43.0f) {

          if (filtered_L > 35.0f) {

              current_state = STATE_TURN_LEFT;

          }

          else if (filtered_R > 35.0f) {

              current_state = STATE_TURN_RIGHT;

          }

          else {

              current_state = STATE_REVERSE_AVOID;

          }

      }

      else {

          current_state = STATE_STRAIGHT;

      }



      // =========================================================

      // 🚗 행동 영역: 모터 제어 로직

      // =========================================================

      switch (current_state)

      {

          case STATE_STRAIGHT:

          {

              throttle = 52; // 고속 주행



              float error = filtered_R - filtered_L;

              float derivative = error - previous_error;



              // 직진 보정 (Kp: 3.5, Kd: 3.5)

              target_steering = (error * 3.5f) + (derivative * 3.5f);

              previous_error = error;



              if (target_steering > 25.0f) target_steering = 25.0f;

              if (target_steering < -25.0f) target_steering = -25.0f;

              break;

          }



          case STATE_TURN_LEFT:

              throttle = 15;

              target_steering = -40.0f;

              break;



          case STATE_TURN_RIGHT:

              throttle = 15;

              target_steering = 40.0f;

              break;



          case STATE_REVERSE_AVOID:

              throttle = -30; // 강력한 후진 브레이크



              if (filtered_C <= 12.0f && filtered_L > 5.0f && filtered_R > 5.0f) {

                  // 정면만 막혔을 때 (넓은 쪽으로 꼬리 빼기)

                  target_steering = (filtered_L > filtered_R) ? 60.0f : -60.0f;

              }

              else if (filtered_L <= 5.0f) {

                  // 왼쪽 벽에 박기 직전: 핸들 왼쪽 꺾고 후진해서 차체 펴기

                  target_steering = -60.0f;

              }

              else if (filtered_R <= 5.0f) {

                  // 오른쪽 벽에 박기 직전: 핸들 오른쪽 꺾고 후진해서 차체 펴기

                  target_steering = 60.0f;

              }

              break;

      }



      // 목표 조향값 즉각 반영

      current_steering = (current_steering * 0.2f) + (target_steering * 0.8f);

      Motor_Drive(throttle, (int)current_steering);



      osDelay(20);

  }

  /* USER CODE END StartMotorTask */
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Function implementing the SensorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */

  for(;;)

  {

        HAL_GPIO_WritePin(Trig_L_GPIO_Port, Trig_L_Pin, GPIO_PIN_SET);

        delay_us(10);

        HAL_GPIO_WritePin(Trig_L_GPIO_Port, Trig_L_Pin, GPIO_PIN_RESET);

        osDelay(15);



        HAL_GPIO_WritePin(Trig_C_GPIO_Port, Trig_C_Pin, GPIO_PIN_SET);

        delay_us(10);

        HAL_GPIO_WritePin(Trig_C_GPIO_Port, Trig_C_Pin, GPIO_PIN_RESET);

        osDelay(15);



        HAL_GPIO_WritePin(Trig_R_GPIO_Port, Trig_R_Pin, GPIO_PIN_SET);

        delay_us(10);

        HAL_GPIO_WritePin(Trig_R_GPIO_Port, Trig_R_Pin, GPIO_PIN_RESET);

        osDelay(15);

  }

  /* USER CODE END StartSensorTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        uint8_t ch_idx = 0;
        uint32_t channel = 0;
        uint32_t *distance_ptr = NULL;

        // 인터럽트가 발생한 채널 식별 및 변수 매핑
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
            ch_idx = 0; channel = TIM_CHANNEL_1; distance_ptr = &distance_L;
        } else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
            ch_idx = 1; channel = TIM_CHANNEL_2; distance_ptr = &distance_C;
        } else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
            ch_idx = 2; channel = TIM_CHANNEL_3; distance_ptr = &distance_R;
        } else {
            return;
        }

        // Echo 핀 상승 에지 (신호 시작)
        if (ic_flag[ch_idx] == 0)
        {
            ic_val1[ch_idx] = HAL_TIM_ReadCapturedValue(htim, channel);
            ic_flag[ch_idx] = 1;
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, channel, TIM_INPUTCHANNELPOLARITY_FALLING);
        }
        // Echo 핀 하강 에지 (신호 종료)
        else if (ic_flag[ch_idx] == 1)
        {
            ic_val2[ch_idx] = HAL_TIM_ReadCapturedValue(htim, channel);

            // 타이머 카운터 오버플로우 고려한 시간차 계산
            uint32_t diff = 0;
            if (ic_val2[ch_idx] > ic_val1[ch_idx]) {
                diff = ic_val2[ch_idx] - ic_val1[ch_idx];
            } else {
                diff = (0xFFFF - ic_val1[ch_idx]) + ic_val2[ch_idx];
            }

            // 거리(cm) 변환 (음속 340m/s 기준)
            *distance_ptr = diff * 0.034f / 2.0f;

            // 다음 측정을 위해 플래그 및 극성 초기화
            ic_flag[ch_idx] = 0;
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, channel, TIM_INPUTCHANNELPOLARITY_RISING);
        }
    }
}
/* USER CODE END Application */

