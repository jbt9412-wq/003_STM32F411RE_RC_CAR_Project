# 003_STM32F411RE_RC_CAR_Project
STM M4 시리즈 RC카 주행(초음파 센서 / 자율 주행 / 수동 주행)
# STM32 FreeRTOS Autonomous 4WD RC Car

STM32F411RE MCU와 FreeRTOS를 기반으로 구현한 4륜 구동(4WD) 자율주행 및 블루투스 원격 제어 RC카 프로젝트입니다. 하드웨어 타이머(Input Capture)를 이용한 3채널 초음파 센서 비동기 처리, L298N 기반의 조향 믹싱 알고리즘, 그리고 RTOS 태스크 스케줄링을 통한 안정적인 자율주행 로직을 구현했습니다.


## 주요 기능 (Key Features)

- **FreeRTOS 기반 멀티태스킹:** 센서 트리거(SensorTask)와 주행 판단 및 모터 제어(MotorTask)를 분리하여 시스템 블로킹 없이 안정적인 주행 제어
- **하드웨어 타이머 기반 거리 측정 (Input Capture):** 3개의 HC-SR04 초음파 센서 에코(Echo) 신호를 TIM3 Input Capture 인터럽트로 수신하여 1us 단위의 정밀한 거리 연산 수행 (소프트웨어 딜레이 배제)
- **자율주행 모드 (Auto Mode):** - 좌/우/중앙 센서의 LPF(Low-Pass Filter) 처리된 데이터를 바탕으로 장애물 회피 기동 (후진, 좌/우 선회)
  - 직진 주행 시 좌우 센서 오차 기반 PD 제어(Proportional-Derivative)를 적용하여 차체 정렬 보정
- **수동 제어 모드 (Manual Mode):** - 스마트폰 앱(Arduino Bluetooth Controller)과 HC-06을 연동하여 UART 수신 인터럽트 기반의 즉각적인 수동 조작
  - 버튼 Release 이벤트를 감지하여 자동 브레이크 동작
- **차동 조향(Differential Steering) 믹싱:** 4WD 모터를 좌/우 2채널로 병렬 병합하고, Throttle과 Steering 값을 믹싱하여 스키드 스티어링(Skid-steer) 구현

## 하드웨어 구성 (Hardware System)

전원 공급의 안정성을 위해 모터 구동부와 로직 제어부의 전원을 분리 및 강하하는 구조를 채택했습니다.

| 분류 | 부품명 | 설명 |
| :--- | :--- | :--- |
| **MCU** | STM32F411RET6 | Nucleo-64 개발 보드 |
| **구동계** | 아두이노 4WD 섀시 | DC 기어드 모터 4개 (좌/우 병렬 결선) |
| **모터 드라이버** | L298N | 듀얼 채널 H-Bridge 드라이버 |
| **주 전원** | 3.6V 배터리 x 3 (직렬) | **10.8V** 주 전원으로 L298N 모터 드라이버 구동 |
| **로직 전원** | L298N 5V Output | 모터 드라이버의 5V 강하 출력을 STM32 Vin에 인가 |
| **센서부** | HC-SR04 x 3 | 전면 좌/중/우 초음파 센서 |
| **통신부** | HC-06 | 블루투스 2.0 SPP 모듈 (스마트폰 통신용) |

## 주변장치 및 핀 매핑 (Pin/Peripheral Mapping)

자세한 설정은 STM32CubeMX `.ioc` 파일에서 관리됩니다.

| 제어 대상 | 주변장치 (Peripheral) | 핀 (Pin) | 설명 |
| :--- | :--- | :--- | :--- |
| **모터 방향 제어** | `GPIO_Output` | `PC0`, `PC1` | 좌측 모터 정/역방향 (L_Fwd, L_Rev) |
| | `GPIO_Output` | `PC2`, `PC3` | 우측 모터 정/역방향 (R_Fwd, R_Rev) |
| **모터 속도 제어** | `TIM2 (PWM)` | `PA0`, `PA1` | 좌/우 모터 속도 제어용 PWM 출력 |
| **초음파 Trigger** | `GPIO_Output` | `PA8`, `PA9`, `PA10`| 좌, 중앙, 우측 트리거 신호 생성 |
| **초음파 Echo** | `TIM3 (Input Capture)`| `PC6`, `PC7`, `PC8` | 에코 신호의 상승/하강 에지 시간차 캡처 |
| **블루투스 제어** | `USART2 (Asynchronous)`| `PA2(TX)`, `PA3(RX)` | 9600 bps 비동기 인터럽트 수신 |
| **시스템 타이머** | `TIM11` | - | FreeRTOS Timebase Source |

## 소프트웨어 아키텍처 (Software Architecture)

시스템은 인터럽트 서비스 루틴(ISR)과 2개의 주요 RTOS 태스크로 구성됩니다.

1. **`SensorTask` (Priority: Normal)**
   - 15ms 주기로 좌, 중, 우 초음파 센서의 Trigger 핀에 10us 펄스를 순차적으로 출력합니다.
   - 대기 시간(Blocking)을 `osDelay`로 처리하여 CPU 자원 점유를 최소화합니다.

2. **`TIM3_IRQHandler` (Input Capture Callback)**
   - 초음파 센서의 Echo 핀 상태가 변할 때마다 하드웨어 인터럽트가 발생합니다.
   - 타이머 카운터(CNT) 값을 읽어 상승 에지와 하강 에지 간의 시간차를 계산하고 거리(cm)로 변환합니다.

3. **`UART2_IRQHandler` (Rx Complete Callback)**
   - 스마트폰으로부터 제어 문자(F, B, L, R, S, A 등)를 수신합니다.
   - 인터럽트 내에서 즉각적으로 `car_mode` 플래그를 변경하고 목표 Throttle/Steering 값을 갱신합니다.

4. **`MotorTask` (Priority: Normal)**
   - `car_mode`가 'Auto(A)'일 경우, 센서 값을 노이즈 필터링(EMA Filter) 한 후 상태 머신(직진/선회/회피)에 따라 조향각과 속도를 자율적으로 연산합니다.
   - `car_mode`가 'Manual(M)'일 경우, 자율 연산을 중지하고 UART로 수신된 목표값을 적용합니다.
   - 최종적으로 연산된 조향/속도 값을 `Motor_Drive()` 믹싱 함수에 전달하여 L298N에 PWM 및 방향 신호를 출력합니다.

## 조작 방법 (How to Play)

스마트폰에서 `Arduino Bluetooth Controller` 앱을 통해 조작 가능합니다. 설정(Settings) 메뉴에서 각 버튼의 전송 값을 아래와 같이 세팅해야 합니다.

* **자율주행/수동 전환:** * 자율주행 모드 진입: `Start` 버튼 누름 (Command: **`A`**)
  * 수동 모드 강제 정지: `Pause` 버튼 누름 (Command: **`P`**)
* **수동 주행 컨트롤 (Press / Release):**
  * 전진 (Up): **`F`** / **`0`**
  * 후진 (Down): **`B`** / **`0`**
  * 좌회전 (Left): **`L`** / **`0`**
  * 우회전 (Right): **`R`** / **`0`**
  * 강제 정지 (Square): **`S`** / **`0`**
* **최고 속도 제어:**
  * 가속 (Triangle): **`T`** (+10%)
  * 감속 (Cross): **`X`** (-10%)

## 개발 환경 (IDE & Tools)

* **IDE:** STM32CubeIDE (v1.14.0+)
* **Firmware Package:** STM32Cube FW_F4 V1.28.3
* **OS:** FreeRTOS (CMSIS-RTOS V2)
* **Code Generation:** STM32CubeMX
