#include "MDR32FxQI_adc.h"
#include "MDR32FxQI_dma.h"

#define ADC_MAX_VALUE      4095
#define VREF               3300        // внутренний Vref MDR
#define LM35_SENSITIVITY   10       // 10 mV / °C

void DMA_IRQHandler(void);
void dmaInit(void);
void adcInit(void);

//uint16_t ADC_GetAverage(void);
//float ADC_ToTemperature(uint16_t adc);
uint16_t Get_Avg_ADC_value(void);
uint16_t ADC_ToTemp(uint16_t adc_val);

