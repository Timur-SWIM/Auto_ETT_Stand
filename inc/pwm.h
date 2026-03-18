#include "MDR32FxQI_port.h"
#include "MDR32FxQI_timer.h"
#include "MDR32FxQI_rst_clk.h"

#define PWM_PORT     MDR_PORTB
#define PWM         PORT_Pin_0

/* PID регулятор */
#define T_MIN 20          // минимальная допустимая температура, °C
#define T_MAX 30          // максимальная допустимая температура, °C
#define PWM_PERIOD 1281   // период ШИМ (ARR+1)
#define PWM_MIN_DUTY 10   // минимальная скважность, %
#define PWM_MAX_DUTY 100  // максимальная скважность, %

/* Коэффициенты PID (эмпирические) */
#define PID_KP 1.0f
#define PID_KI 0.1f
#define PID_KD 0.01f

void PortInit(void);
void TimerInit(void);
void PWM_SetDutyCycle(uint16_t duty_percent); // установка скважности в процентах
void PID_Update(int16_t current_temp);        // обновление PID и скважности
