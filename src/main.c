/* Includes ------------------------------------------------------------------*/
#include "main.h"

void ClockInit(void){
    RST_CLK_DeInit();
    SystemCoreClockUpdate();

	RST_CLK_HSEconfig(RST_CLK_HSE_ON);
	
	if (RST_CLK_HSEstatus() != SUCCESS) {
		while(1);
	}
	
	RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSEdiv1, RST_CLK_CPU_PLLmul4);
	
	RST_CLK_CPU_PLLcmd(ENABLE);

    // Wait for CPU PLL to lock
    if (RST_CLK_CPU_PLLstatus() != SUCCESS) {        
        /* Trap if PLL is not successfully locked */        
        while (1);    
    }
    // Set CPU clock prescaler to 1 (no division)
    RST_CLK_CPUclkPrescaler(RST_CLK_CPUclkDIV1);

    // Enable CPU PLL as the clock source
    RST_CLK_CPU_PLLuse(ENABLE);
	
    /* Enables the RST_CLK_PCLK_EEPROM */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_EEPROM, ENABLE);
    /* Sets the code latency value (CPU_CLK up to 100 MHz) */
    EEPROM_SetLatency(EEPROM_Latency_3);
    /* Disables the RST_CLK_PCLK_EEPROM */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_EEPROM, DISABLE);
    
    /* Enables the RST_CLK_PCLK_BKP */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_BKP, ENABLE);
    /* Setting the parameters of the voltage regulator SelectRI and LOW in the BKP controller (CPU_CLK = 80 MHz) */
    BKP_DUccMode(BKP_DUcc_upto_80MHz);	

    // Select CPU clock C3 (PLL output as the CPU clock source)
    RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);
	
	RST_CLK_PCLKcmd((RST_CLK_PCLK_RST_CLK | 
	                 RST_CLK_PCLK_SSP1 | 
	                 RST_CLK_PCLK_SSP2), 
	                 ENABLE);
}

#define BUFFER_LENGTH   100
static uint8_t Buffer[BUFFER_LENGTH];
int main(void) {
	ClockInit();
	dmaInit();
	adcInit();
	PortInit();
	TimerInit();
    LedPortInit();
    Timer_1_Init();
    DAC_DMA_Init();
	VCom_Configuration();
	USB_CDC_Init(Buffer, 1 , SET);
	Setup_USB();
	while(1) {
        uint16_t adc_val = Get_Avg_ADC_value();
		uint16_t temp = ADC_ToTemp(adc_val);
        // Обновление скважности ШИМ через PID регулятор
		PID_Update((int16_t)temp);
		if(usb_transmit_flag) {
			usb_transmit_flag = 0;
			USB_SendTemp(temp);
		}
	}
}



/**
  * @brief  Reports the source file name, the source line number
  *         and expression text (if USE_ASSERT_INFO == 2) where
  *         the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @param  expr:
  * @retval None
  */
#if (USE_ASSERT_INFO == 1)
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the source file name and line number.
       Ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while(1) {}
}
#elif (USE_ASSERT_INFO == 2)
void assert_failed(uint8_t* file, uint32_t line, const uint8_t* expr)
{
    /* User can add his own implementation to report the source file name, line number and
       expression text.
       Ex: printf("Wrong parameters value (%s): file %s on line %d\r\n", expr, file, line) */

    /* Infinite loop */
    while(1) {}
}
#endif /* USE_ASSERT_INFO */

/** @} */ /* End of group ADC_ADC1_DMA_MDR32F9Q2I */

/** @} */ /* End of group __MDR32F9Q2I_EVAL */

/** @} */ /* End of group __MDR32FxQI_StdPeriph_Examples */

/******************* (C) COPYRIGHT 2025 Milandr *******************************/

/* END OF FILE main.c */

