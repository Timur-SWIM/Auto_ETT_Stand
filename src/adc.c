#include "adc.h"

ADC_InitTypeDef sADC;
ADCx_InitTypeDef sADCx;
uint16_t ADCConvertedValue[10];
DMA_ChannelInitTypeDef DMA_InitStr;
DMA_CtrlDataInitTypeDef DMA_PriCtrlStr;
DMA_CtrlDataInitTypeDef DMA_AltCtrlStr;

void dmaInit(void) {
    /* Disable all interrupt */
    NVIC->ICPR[0] = 0xFFFFFFFF;
    NVIC->ICER[0] = 0xFFFFFFFF;
	/* DMA Configuration */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_DMA, ENABLE);
    /* Reset all settings */
    DMA_DeInit();

    /* Set Primary Control Data */
    DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)(&(MDR_ADC->ADC2_RESULT));
    DMA_PriCtrlStr.DMA_DestBaseAddr   = (uint32_t)ADCConvertedValue;
    DMA_PriCtrlStr.DMA_SourceIncSize  = DMA_SourceIncNo;
    DMA_PriCtrlStr.DMA_DestIncSize    = DMA_DestIncHalfword;
    DMA_PriCtrlStr.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_PriCtrlStr.DMA_Mode           = DMA_Mode_PingPong;
    DMA_PriCtrlStr.DMA_CycleSize      = 10;
    DMA_PriCtrlStr.DMA_NumContinuous  = DMA_Transfers_1;
    DMA_PriCtrlStr.DMA_SourceProtCtrl = DMA_SourcePrivileged;
    DMA_PriCtrlStr.DMA_DestProtCtrl   = DMA_DestPrivileged;

    /* Set Alternate Control Data */
    DMA_AltCtrlStr.DMA_SourceBaseAddr = (uint32_t)(&(MDR_ADC->ADC2_RESULT));
    DMA_AltCtrlStr.DMA_DestBaseAddr   = (uint32_t)ADCConvertedValue;
    DMA_AltCtrlStr.DMA_SourceIncSize  = DMA_SourceIncNo;
    DMA_AltCtrlStr.DMA_DestIncSize    = DMA_DestIncHalfword;
    DMA_AltCtrlStr.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_AltCtrlStr.DMA_Mode           = DMA_Mode_PingPong;
    DMA_AltCtrlStr.DMA_CycleSize      = 10;
    DMA_AltCtrlStr.DMA_NumContinuous  = DMA_Transfers_1;
    DMA_AltCtrlStr.DMA_SourceProtCtrl = DMA_SourcePrivileged;
    DMA_AltCtrlStr.DMA_DestProtCtrl   = DMA_DestPrivileged;

    /* Set Channel Structure */
    DMA_StructInit(&DMA_InitStr);
    DMA_InitStr.DMA_PriCtrlData         = &DMA_PriCtrlStr;
    DMA_InitStr.DMA_AltCtrlData         = &DMA_AltCtrlStr;
    DMA_InitStr.DMA_Priority            = DMA_Priority_Default;
    DMA_InitStr.DMA_UseBurst            = DMA_BurstClear;
    DMA_InitStr.DMA_SelectDataStructure = DMA_CTRL_DATA_PRIMARY;

    /* Init DMA channel ADC2 */
    DMA_Init(DMA_Channel_ADC2, &DMA_InitStr);

    /* Enable DMA channel ADC2 */
    DMA_Cmd(DMA_Channel_ADC2, ENABLE);	


}

void adcInit(void) {
    /* ADC Configuration */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_ADC, ENABLE);
    /* Reset all ADC settings */
    ADC_DeInit();

    ADC_StructInit(&sADC);
    sADC.ADC_SynchronousMode      = ADC_SyncMode_Independent;
    sADC.ADC_StartDelay           = 0;
    sADC.ADC_TempSensor           = ADC_TEMP_SENSOR_Enable;
    sADC.ADC_TempSensorAmplifier  = ADC_TEMP_SENSOR_AMPLIFIER_Disable;
    sADC.ADC_TempSensorConversion = ADC_TEMP_SENSOR_CONVERSION_Disable;
    sADC.ADC_IntVRefConversion    = ADC_VREF_CONVERSION_Disable;
    sADC.ADC_IntVRefTrimming      = 1;
    ADC_Init(&sADC);

    /* ADC1 Configuration */
    ADCx_StructInit (&sADCx);
    sADCx.ADC_ClockSource      = ADC_CLOCK_SOURCE_CPU;
    sADCx.ADC_SamplingMode     = ADC_SAMPLING_MODE_CYCLIC_CONV;
    sADCx.ADC_ChannelSwitching = ADC_CH_SWITCHING_Disable;
    sADCx.ADC_ChannelNumber    = ADC_CH_ADC2;
    sADCx.ADC_Channels         = 0;
    sADCx.ADC_LevelControl     = ADC_LEVEL_CONTROL_Disable;
    sADCx.ADC_LowLevel         = 0;
    sADCx.ADC_HighLevel        = 0;
    sADCx.ADC_VRefSource       = ADC_VREF_SOURCE_INTERNAL;
    sADCx.ADC_IntVRefSource    = ADC_INT_VREF_SOURCE_INEXACT;
    sADCx.ADC_Prescaler        = ADC_CLK_div_64;
    sADCx.ADC_DelayGo          = 7;
    ADC2_Init(&sADCx);

    /* Disable ADC1 EOCIF and AWOIFEN interupts */
    ADC2_ITConfig((ADC2_IT_END_OF_CONVERSION | ADC2_IT_OUT_OF_RANGE), DISABLE);

    /* ADC1 enable */
    ADC2_Cmd(ENABLE);	
	/* Enable DMA IRQ */
    NVIC_EnableIRQ(DMA_IRQn);
}

uint16_t Get_Avg_ADC_value(void) {
    uint32_t sum = 0;

    for(int i = 0; i < 10; i++) {
        sum += ADCConvertedValue[i]; }
    
    uint16_t average_adc = sum / 10;
    return average_adc;
}   
uint16_t ADC_ToTemp(uint16_t adc_val) {
	uint16_t voltage = (adc_val * VREF) / ADC_MAX_VALUE;
	uint16_t temp = voltage / LM35_SENSITIVITY;
	return temp;
}


