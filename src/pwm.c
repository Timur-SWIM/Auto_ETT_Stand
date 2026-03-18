#include "pwm.h"

/* Private variables ---------------------------------------------------------*/
PORT_InitTypeDef PORT_InitStructure;
TIMER_CntInitTypeDef sTIM_CntInit;
TIMER_ChnInitTypeDef sTIM_ChnInit;
TIMER_ChnOutInitTypeDef sTIM_ChnOutInit;

uint16_t CCR1_Val = 600+1;

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