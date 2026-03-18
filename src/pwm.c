#include "pwm.h"

/* Private variables ---------------------------------------------------------*/
PORT_InitTypeDef PORT_InitStructure;
TIMER_CntInitTypeDef sTIM_CntInit;
TIMER_ChnInitTypeDef sTIM_ChnInit;
TIMER_ChnOutInitTypeDef sTIM_ChnOutInit;

uint16_t CCR1_Val = 600+1;

/* PID регулятор */
static float pid_integral = 0.0f;
static float pid_prev_error = 0.0f;
static const int16_t pid_setpoint = (T_MIN + T_MAX) / 2; // целевая температура

void PortInit(void) {
    /* Enable peripheral clocks */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTB, ENABLE);

    /* Reset PORTB settings */
    PORT_DeInit(PWM_PORT);	
	
	/* Configure PORTB pin0 */
	PORT_StructInit(&PORT_InitStructure);
	PORT_InitStructure.PORT_Pin = PWM;
	PORT_InitStructure.PORT_OE = PORT_OE_OUT;
	PORT_InitStructure.PORT_FUNC = PORT_FUNC_ALTER;
	PORT_InitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_FAST;
	
	PORT_Init(PWM_PORT, &PORT_InitStructure);
}

void TimerInit(void) {
	/* Enable peripheral clocks */
	RST_CLK_PCLKcmd(RST_CLK_PCLK_TIMER3, ENABLE);
	/* Reset all TIM3 settings */
	TIMER_DeInit(MDR_TIMER3);
	
	/* TIM3 config:
		Genreate PWM signal:
		TIM3CLK = 32MHz, Prescaller = 0, TIM3 counter clock = ?MHz
		TIM3 frequency = TIM3CLK/(TIM3_Period + 1) = ? KHz
		- TIM3 Channel4 duty cycle = TIM3->CCR1 / (TIM3_Period + 1) = ?%
	*/
	
	/* Init TIMx Counter */
	sTIM_CntInit.TIMER_Prescaler = 0;
	sTIM_CntInit.TIMER_Period    = 1280+1;
	sTIM_CntInit.TIMER_CounterMode = TIMER_CntMode_ClkFixedDir;
	sTIM_CntInit.TIMER_CounterDirection = TIMER_CntDir_Up;
    sTIM_CntInit.TIMER_EventSource      = TIMER_EvSrc_TIM_CLK;
    sTIM_CntInit.TIMER_FilterSampling   = TIMER_FDTS_TIMER_CLK_div_1;
    sTIM_CntInit.TIMER_ARR_UpdateMode   = TIMER_ARR_Update_Immediately;
    sTIM_CntInit.TIMER_ETR_FilterConf   = TIMER_Filter_1FF_at_TIMER_CLK;
    sTIM_CntInit.TIMER_ETR_Prescaler    = TIMER_ETR_Prescaler_None;
    sTIM_CntInit.TIMER_ETR_Polarity     = TIMER_ETRPolarity_NonInverted;
    sTIM_CntInit.TIMER_BRK_Polarity     = TIMER_BRKPolarity_NonInverted;
    TIMER_CntInit(MDR_TIMER3,&sTIM_CntInit);

    /* Initializes the TIMER3 Channel 1 */
    TIMER_ChnStructInit(&sTIM_ChnInit);

    sTIM_ChnInit.TIMER_CH_Mode       = TIMER_CH_MODE_PWM;
    sTIM_ChnInit.TIMER_CH_REF_Format = TIMER_CH_REF_Format6;
    sTIM_ChnInit.TIMER_CH_Number     = TIMER_CHANNEL1;
    TIMER_ChnInit(MDR_TIMER3, &sTIM_ChnInit);

    TIMER_SetChnCompare(MDR_TIMER3, TIMER_CHANNEL1, CCR1_Val);
	
    /* Initializes the TIMER3 Channel 1 */
    TIMER_ChnOutStructInit(&sTIM_ChnOutInit);
    sTIM_ChnOutInit.TIMER_CH_DirOut_Polarity = TIMER_CHOPolarity_NonInverted;
    sTIM_ChnOutInit.TIMER_CH_DirOut_Source   = TIMER_CH_OutSrc_REF;
    sTIM_ChnOutInit.TIMER_CH_DirOut_Mode     = TIMER_CH_OutMode_Output;
    sTIM_ChnOutInit.TIMER_CH_NegOut_Polarity = TIMER_CHOPolarity_NonInverted;
    sTIM_ChnOutInit.TIMER_CH_NegOut_Source   = TIMER_CH_OutSrc_REF;
    sTIM_ChnOutInit.TIMER_CH_NegOut_Mode     = TIMER_CH_OutMode_Output;
    sTIM_ChnOutInit.TIMER_CH_Number          = TIMER_CHANNEL1;
    TIMER_ChnOutInit(MDR_TIMER3, &sTIM_ChnOutInit);	
	
    /* Enable TIMER3 clock */
    TIMER_BRGInit(MDR_TIMER3,TIMER_HCLKdiv1);

    /* Enable TIMER3 */
    TIMER_Cmd(MDR_TIMER3,ENABLE);
}
/**
  * @brief Установка скважности ШИМ в процентах
  * @param duty_percent Скважность от 0 до 100
  */
void PWM_SetDutyCycle(uint16_t duty_percent) {
    // Ограничение по мёртвой зоне: не менее PWM_MIN_DUTY и не более PWM_MAX_DUTY
    if (duty_percent < PWM_MIN_DUTY) {
        duty_percent = PWM_MIN_DUTY;
    } else if (duty_percent > PWM_MAX_DUTY) {
        duty_percent = PWM_MAX_DUTY;
    }
    
    // Перевод процентов в значение CCR1
    // CCR1 = (duty_percent * (PWM_PERIOD - 1)) / 100
    uint32_t ccr = ((uint32_t)duty_percent * (PWM_PERIOD - 1)) / 100;
    CCR1_Val = (uint16_t)ccr;
    
    // Обновление регистра сравнения таймера
    TIMER_SetChnCompare(MDR_TIMER3, TIMER_CHANNEL1, CCR1_Val);
}

/**
  * @brief Обновление состояния PID регулятора и скважности ШИМ
  * @param current_temp Текущая температура в градусах Цельсия
  */
void PID_Update(int16_t current_temp) {
    float error = (float)(pid_setpoint - current_temp);
    
    // Пропорциональная составляющая
    float p_term = PID_KP * error;
    
    // Интегральная составляющая
    pid_integral += error;
    // Антивинд‑ап: ограничение интеграла
    const float integral_limit = 100.0f;
    if (pid_integral > integral_limit) pid_integral = integral_limit;
    if (pid_integral < -integral_limit) pid_integral = -integral_limit;
    float i_term = PID_KI * pid_integral;
    
    // Дифференциальная составляющая
    float d_term = PID_KD * (error - pid_prev_error);
    pid_prev_error = error;
    
    // Суммарное управляющее воздействие
    float output = p_term + i_term + d_term;
    
    // Преобразование в скважность (output - температура, нужно маппировать на проценты)
    // Допустим, output в диапазоне [-50, +50] градусов, преобразуем в проценты:
    // Скважность = 50% + output * коэффициент
    // Коэффициент выберем так, чтобы изменение на 1 градус давало изменение скважности на 2%
    const float scale = 2.0f;
    float duty = 50.0f + output * scale;
    
    // Ограничение
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 100.0f) duty = 100.0f;
    
    PWM_SetDutyCycle((uint16_t)duty);
}