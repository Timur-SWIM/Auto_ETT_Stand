#include "timer.h"

PORT_InitTypeDef LED_PORT_InitStructure;
TIMER_CntInitTypeDef sTIM_1_CntInit;
TIMER_ChnInitTypeDef sTIM_1_ChnInit;
TIMER_ChnOutInitTypeDef sTIM_1_ChnOutInit;

static volatile uint32_t LedState = 0;
volatile uint8_t usb_transmit_flag = 0;

void LedPortInit(void) {
    /* Enable peripheral clocks */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTB, ENABLE);

    /* Reset PORTB settings - temporarily disabled due to crash */
    //PORT_DeInit(LED_PORT);
	
	/* Configure PORTB pin7 */
	PORT_StructInit(&LED_PORT_InitStructure);
	LED_PORT_InitStructure.PORT_Pin = LED;
	LED_PORT_InitStructure.PORT_OE = PORT_OE_OUT;
	LED_PORT_InitStructure.PORT_FUNC = PORT_FUNC_PORT;
	LED_PORT_InitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	LED_PORT_InitStructure.PORT_SPEED = PORT_SPEED_FAST;
	
	PORT_Init(LED_PORT, &LED_PORT_InitStructure);
    PORT_SetBits(LED_PORT, LED);
}

void Timer_1_Init(void) {
	/* Enable peripheral clocks */
	RST_CLK_PCLKcmd(RST_CLK_PCLK_TIMER1, ENABLE);
	/* Reset all TIM1 settings */
	TIMER_DeInit(MDR_TIMER1);
	
	/* Set TIMER clock source (HCLK divider) */
	TIMER_BRGInit(MDR_TIMER1, TIMER_HCLKdiv1);

	/* Init TIMx Counter */
	sTIM_1_CntInit.TIMER_IniCounter        = 0;
	sTIM_1_CntInit.TIMER_Prescaler         = 8000 - 1;        /* Prescaler = 8000 - 1 */
	sTIM_1_CntInit.TIMER_Period            = 8000 - 1;        /* Period = 4000 - 1 (1 second) */
	sTIM_1_CntInit.TIMER_CounterMode       = TIMER_CntMode_ClkFixedDir;
	sTIM_1_CntInit.TIMER_CounterDirection  = TIMER_CntDir_Up;
    sTIM_1_CntInit.TIMER_EventSource       = TIMER_EvSrc_TIM_CLK;
    sTIM_1_CntInit.TIMER_FilterSampling    = TIMER_FDTS_TIMER_CLK_div_1;
    sTIM_1_CntInit.TIMER_ARR_UpdateMode    = TIMER_ARR_Update_Immediately;
    sTIM_1_CntInit.TIMER_ETR_FilterConf    = TIMER_Filter_1FF_at_TIMER_CLK;
    sTIM_1_CntInit.TIMER_ETR_Prescaler     = TIMER_ETR_Prescaler_None;
    sTIM_1_CntInit.TIMER_ETR_Polarity      = TIMER_ETRPolarity_NonInverted;
    sTIM_1_CntInit.TIMER_BRK_Polarity      = TIMER_BRKPolarity_NonInverted;
    TIMER_CntInit(MDR_TIMER1, &sTIM_1_CntInit);
    
    /* Clear pending flag and enable interrupt */
    TIMER_ClearFlag(MDR_TIMER1, TIMER_STATUS_CNT_ZERO);
    TIMER_ITConfig(MDR_TIMER1, TIMER_STATUS_CNT_ZERO, ENABLE);

    /* Enable TIMER1 */
    TIMER_Cmd(MDR_TIMER1, ENABLE);
    NVIC_EnableIRQ(Timer1_IRQn);
}

void Timer1_IRQHandler(void)
{
    /* Проверка флага обновления счётчика */
    if (TIMER_GetFlagStatus(MDR_TIMER1, TIMER_STATUS_CNT_ZERO))
    {
        /* Очистка флага */
        TIMER_ClearFlag(MDR_TIMER1, TIMER_STATUS_CNT_ZERO);

        /* Переключение состояния светодиода на PB7 */
        if (LedState == 0)
        {
            PORT_SetBits(LED_PORT, LED);   /* Зажечь */
            LedState = 1;
        }
        else
        {
            PORT_ResetBits(LED_PORT, LED); /* Погасить */
            LedState = 0;
        }

        /* Установка флага для передачи по USB */
        usb_transmit_flag = 1;
    }
}