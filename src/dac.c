#include "dac.h"

/* Private variables ---------------------------------------------------------*/
PORT_InitTypeDef DAC_PORT_InitStructure;
TIMER_CntInitTypeDef DAC_TIM_CntInit;
DMA_ChannelInitTypeDef DAC_DMA_InitStr;
DMA_CtrlDataInitTypeDef DAC_DMA_PriCtrlStr;
DMA_CtrlDataInitTypeDef DAC_DMA_AltCtrlStr;

uint16_t DAC_DMA_Data[DAC_BUFFER_SIZE] = {
    112,   // 0.09 В = 1500 МГц
    298,   // 0.24 В = 1600 МГц
    521,   // 0.42 В = 1700 МГц
    763,   // 0.615 В = 1800 МГц
    1030,  // 0.83 В = 1900 МГц
    1303,  // 1.05 В = 2000 МГц
    1613,  // 1.3 В = 2100 МГц
    1985,  // 1.6 В = 2200 МГц
    2606,  // 2.1 В = 2300 МГц
    3723   // 3.0 В = 2400 МГц
    };

uint16_t DAC_DMA_Buffer[2][DAC_BUFFER_SIZE];

volatile uint8_t DAC_DMA_ActiveBufferIndex  = 0U; /* 0 -> PRIMARY, 1 -> ALTERNATE */
volatile uint8_t DAC_DMA_UpdatePending      = 0U;
volatile uint8_t DAC_DMA_PendingBufferIndex = 1U;

uint8_t DAC_GetInactiveBufferIndex(void)
{
    return (DAC_DMA_ActiveBufferIndex == 0U) ? 1U : 0U;
}

int DAC_RequestBufferUpdate(const uint16_t *src, uint32_t count)
{
    uint8_t inactive_index;

    if ((src == 0) || (count != DAC_BUFFER_SIZE)) {
        return -1;
    }

    NVIC_DisableIRQ(DMA_IRQn);

    inactive_index = DAC_GetInactiveBufferIndex();

    memcpy(DAC_DMA_Buffer[inactive_index], src, DAC_BUFFER_SIZE * sizeof(uint16_t));

    DAC_DMA_PendingBufferIndex = inactive_index;
    DAC_DMA_UpdatePending      = 1U;

    NVIC_EnableIRQ(DMA_IRQn);

    return 0;
}

int DAC_DMA_Init(void) {
    /* Initialize DMA */
    My_DMA_Init();
    /* Initialize DAC */
    My_DAC_Init();
    /* Initialize TIMER */
    My_TIMER_Init();
    return 0;
}

int My_DMA_Init(void) {
    memcpy(DAC_DMA_Buffer[0], DAC_DMA_Data, sizeof(DAC_DMA_Data));
    memcpy(DAC_DMA_Buffer[1], DAC_DMA_Data, sizeof(DAC_DMA_Data));

    DAC_DMA_ActiveBufferIndex  = 0U;
    DAC_DMA_PendingBufferIndex = 1U;
    DAC_DMA_UpdatePending      = 0U;
    //RST_CLK_PCLKcmd(RST_CLK_PCLK_DAC, ENABLE);
    /* DMA Configuration */
    DAC_DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)DAC_DMA_Buffer[0];
    DAC_DMA_PriCtrlStr.DMA_DestBaseAddr   = (uint32_t)(&(MDR_DAC->DAC2_DATA));
    DAC_DMA_PriCtrlStr.DMA_SourceIncSize  = DMA_SourceIncHalfword;
    DAC_DMA_PriCtrlStr.DMA_DestIncSize    = DMA_DestIncNo;
    DAC_DMA_PriCtrlStr.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DAC_DMA_PriCtrlStr.DMA_Mode           = DMA_Mode_PingPong;
    DAC_DMA_PriCtrlStr.DMA_CycleSize      = 10;
    DAC_DMA_PriCtrlStr.DMA_NumContinuous  = DMA_Transfers_1;
    DAC_DMA_PriCtrlStr.DMA_SourceProtCtrl = DMA_SourcePrivileged;   
    DAC_DMA_PriCtrlStr.DMA_DestProtCtrl   = DMA_DestPrivileged;
    /* Set Alternate Control Data */
    DAC_DMA_AltCtrlStr.DMA_SourceBaseAddr = (uint32_t)DAC_DMA_Buffer[1];
    DAC_DMA_AltCtrlStr.DMA_DestBaseAddr   = (uint32_t)(&(MDR_DAC->DAC2_DATA));
    DAC_DMA_AltCtrlStr.DMA_SourceIncSize  = DMA_SourceIncHalfword;
    DAC_DMA_AltCtrlStr.DMA_DestIncSize    = DMA_DestIncNo;
    DAC_DMA_AltCtrlStr.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DAC_DMA_AltCtrlStr.DMA_Mode           = DMA_Mode_PingPong;
    DAC_DMA_AltCtrlStr.DMA_CycleSize      = 10;
    DAC_DMA_AltCtrlStr.DMA_NumContinuous  = DMA_Transfers_1;
    DAC_DMA_AltCtrlStr.DMA_SourceProtCtrl = DMA_SourcePrivileged;
    DAC_DMA_AltCtrlStr.DMA_DestProtCtrl   = DMA_DestPrivileged;
    /* Set Channel Structure */
    DMA_StructInit(&DAC_DMA_InitStr);
    DAC_DMA_InitStr.DMA_PriCtrlData         = &DAC_DMA_PriCtrlStr;
    DAC_DMA_InitStr.DMA_AltCtrlData         = &DAC_DMA_AltCtrlStr;
    DAC_DMA_InitStr.DMA_Priority            = DMA_Priority_Default;
    DAC_DMA_InitStr.DMA_UseBurst            = DMA_BurstClear;
    DAC_DMA_InitStr.DMA_SelectDataStructure = DMA_CTRL_DATA_PRIMARY;
    /* Init DMA channel TIM2*/
    DMA_Init(DMA_Channel_TIM2, &DAC_DMA_InitStr);
    /* Enable DMA_Channel_TIM2 */
    DMA_Cmd(DMA_Channel_TIM2, ENABLE);
    return 0;   
}

int My_TIMER_Init(void) {
    /* Enable peripheral clocks */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTE |
                    RST_CLK_PCLK_TIMER2, ENABLE);
    /* Reset PORTE settings */
    PORT_DeInit(MDR_PORTE);
    /* Configure DAC pin: DAC2_OUT */
    /* Configure PORTE pin 0 */
    PORT_StructInit(&DAC_PORT_InitStructure);
    DAC_PORT_InitStructure.PORT_Pin  = PORT_Pin_0;
    DAC_PORT_InitStructure.PORT_OE   = PORT_OE_OUT;
    DAC_PORT_InitStructure.PORT_MODE = PORT_MODE_ANALOG;
    PORT_Init(MDR_PORTE, &DAC_PORT_InitStructure);    
    /* TIMER2 Configuration */
    /* Time base configuration */
    TIMER_DeInit(MDR_TIMER2);
    TIMER_BRGInit(MDR_TIMER2,TIMER_HCLKdiv1);
    DAC_TIM_CntInit.TIMER_Prescaler        = 0x001F;
    DAC_TIM_CntInit.TIMER_Period           = 0x0063;
    DAC_TIM_CntInit.TIMER_CounterMode      = TIMER_CntMode_ClkFixedDir;
    DAC_TIM_CntInit.TIMER_CounterDirection = TIMER_CntDir_Up;
    DAC_TIM_CntInit.TIMER_EventSource      = TIMER_EvSrc_TIM_CLK;
    DAC_TIM_CntInit.TIMER_FilterSampling   = TIMER_FDTS_TIMER_CLK_div_1;
    DAC_TIM_CntInit.TIMER_ARR_UpdateMode   = TIMER_ARR_Update_Immediately;
    DAC_TIM_CntInit.TIMER_ETR_FilterConf   = TIMER_Filter_1FF_at_TIMER_CLK;
    DAC_TIM_CntInit.TIMER_ETR_Prescaler    = TIMER_ETR_Prescaler_None;
    DAC_TIM_CntInit.TIMER_ETR_Polarity     = TIMER_ETRPolarity_NonInverted;
    DAC_TIM_CntInit.TIMER_BRK_Polarity     = TIMER_BRKPolarity_NonInverted;
    TIMER_CntInit(MDR_TIMER2, &DAC_TIM_CntInit);
    /* Enable DMA for TIMER1 */
    TIMER_DMACmd(MDR_TIMER2, TIMER_STATUS_CNT_ARR, ENABLE);
    /* TIMER1 enable counter */
    TIMER_Cmd(MDR_TIMER2, ENABLE);
    /* Enable DMA IRQ */
    //NVIC_EnableIRQ(DMA_IRQn);
    return 0;
}

int My_DAC_Init(void) {
    /* Enable peripheral clocks */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_DAC, ENABLE);
    /* DAC Configuration */
    /* Reset all DAC settings */
    DAC_DeInit();
    /* DAC channel2 Configuration */
    DAC2_Init(DAC2_AVCC);
    /* DAC channel2 enable */
    DAC2_Cmd(ENABLE);
    return 0;
}
