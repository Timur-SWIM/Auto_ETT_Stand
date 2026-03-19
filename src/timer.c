#include "timer.h"

PORT_InitTypeDef PORT_InitStructure;
TIMER_CntInitTypeDef sTIM_CntInit;
TIMER_ChnInitTypeDef sTIM_ChnInit;
TIMER_ChnOutInitTypeDef sTIM_ChnOutInit;

static volatile uint32_t LedState = 0;

void LedPortInit(void) {
    /* Enable peripheral clocks */
    //RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTB, ENABLE);

    /* Reset PORTB settings */
    //PORT_DeInit(LED_PORT);	
	
	/* Configure PORTB pin7 */
	PORT_StructInit(&PORT_InitStructure);
	PORT_InitStructure.PORT_Pin = LED;
	PORT_InitStructure.PORT_OE = PORT_OE_OUT;
	PORT_InitStructure.PORT_FUNC = PORT_FUNC_ALTER;
	PORT_InitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_FAST;
	
	PORT_Init(LED_PORT, &PORT_InitStructure);
}

void Timer_1_Init(void) {
	/* Enable peripheral clocks */
	RST_CLK_PCLKcmd(RST_CLK_PCLK_TIMER1, ENABLE);
	/* Reset all TIM1 settings */
	TIMER_DeInit(MDR_TIMER1);
	
	/* Init TIMx Counter */
	sTIM_CntInit.TIMER_Prescaler = 32000-1; 
	sTIM_CntInit.TIMER_Period    = 1000+1;
	sTIM_CntInit.TIMER_CounterMode = TIMER_CntMode_ClkFixedDir;
	sTIM_CntInit.TIMER_CounterDirection = TIMER_CntDir_Up;
    sTIM_CntInit.TIMER_EventSource      = TIMER_EvSrc_TIM_CLK;
    sTIM_CntInit.TIMER_FilterSampling   = TIMER_FDTS_TIMER_CLK_div_1;
    sTIM_CntInit.TIMER_ARR_UpdateMode   = TIMER_ARR_Update_Immediately;
    sTIM_CntInit.TIMER_ETR_FilterConf   = TIMER_Filter_1FF_at_TIMER_CLK;
    sTIM_CntInit.TIMER_ETR_Prescaler    = TIMER_ETR_Prescaler_None;
    sTIM_CntInit.TIMER_ETR_Polarity     = TIMER_ETRPolarity_NonInverted;
    sTIM_CntInit.TIMER_BRK_Polarity     = TIMER_BRKPolarity_NonInverted;
    TIMER_CntInit(MDR_TIMER1,&sTIM_CntInit);

    /* Initializes the TIMER1 Channel 1 */
    TIMER_ChnStructInit(&sTIM_ChnInit);

    sTIM_ChnInit.TIMER_CH_Mode       = TIMER_CH_MODE_CAPTURE;
    sTIM_ChnInit.TIMER_CH_REF_Format = TIMER_CH_REF_Format6;
    sTIM_ChnInit.TIMER_CH_Number     = TIMER_CHANNEL1;
    TIMER_ChnInit(MDR_TIMER1, &sTIM_ChnInit);

    //TIMER_SetChnCompare(MDR_TIMER1, TIMER_CHANNEL1, CCR1_Val);
	
    /* Initializes the TIMER1 Channel 1 */
    // TIMER_ChnOutStructInit(&sTIM_ChnOutInit);
    // sTIM_ChnOutInit.TIMER_CH_DirOut_Polarity = TIMER_CHOPolarity_NonInverted;
    // sTIM_ChnOutInit.TIMER_CH_DirOut_Source   = TIMER_CH_OutSrc_REF;
    // sTIM_ChnOutInit.TIMER_CH_DirOut_Mode     = TIMER_CH_OutMode_Output;
    // sTIM_ChnOutInit.TIMER_CH_NegOut_Polarity = TIMER_CHOPolarity_NonInverted;
    // sTIM_ChnOutInit.TIMER_CH_NegOut_Source   = TIMER_CH_OutSrc_REF;
    // sTIM_ChnOutInit.TIMER_CH_NegOut_Mode     = TIMER_CH_OutMode_Output;
    // sTIM_ChnOutInit.TIMER_CH_Number          = TIMER_CHANNEL1;
    // TIMER_ChnOutInit(MDR_TIMER1, &sTIM_ChnOutInit);	

    /* Enable TIMER1 clock */
    TIMER_BRGInit(MDR_TIMER1,TIMER_HCLKdiv1);

    TIMER_ClearFlag(MDR_TIMER1, TIMER_STATUS_CNT_ARR);
    TIMER_ITConfig(MDR_TIMER1, TIMER_IE_CNT_ARR_EVENT_IE, ENABLE);

    /* Enable TIMER1 */
    TIMER_Cmd(MDR_TIMER1,ENABLE);
    NVIC_EnableIRQ(Timer1_IRQn);
}

void TIMER1_IRQHandler(void)
{
    /* Проверка флага обновления счётчика */
    if (TIMER_GetFlagStatus(MDR_TIMER1, TIMER_STATUS_CNT_ARR) == SET)
    {
        /* Очистка флага */
        TIMER_ClearFlag(MDR_TIMER1, TIMER_STATUS_CNT_ARR);

        /* Переключение состояния светодиода на PB7 */
        if (LedState == 0)
        {
            PORT_SetBits(MDR_PORTB, PORT_Pin_7);   /* Зажечь */
            LedState = 1;
        }
        else
        {
            PORT_ResetBits(MDR_PORTB, PORT_Pin_7); /* Погасить */
            LedState = 0;
        }
    }
}