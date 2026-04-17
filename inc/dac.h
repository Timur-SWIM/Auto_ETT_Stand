#include "MDR32FxQI_port.h"
#include "MDR32FxQI_rst_clk.h"
#include "MDR32FxQI_dac.h"
#include "MDR32FxQI_timer.h"
#include "MDR32FxQI_dma.h"
#include <stdint.h>
#include <string.h>

#define DAC_BUFFER_SIZE 10U

extern uint16_t DAC_DMA_Data[DAC_BUFFER_SIZE];
extern uint16_t DAC_DMA_Buffer[2][DAC_BUFFER_SIZE];

extern volatile uint8_t DAC_DMA_ActiveBufferIndex;
extern volatile uint8_t DAC_DMA_UpdatePending;
extern volatile uint8_t DAC_DMA_PendingBufferIndex;

int DAC_DMA_Init(void);
int My_DMA_Init(void);
int My_TIMER_Init(void);
int My_DAC_Init(void);

int DAC_RequestBufferUpdate(const uint16_t *src, uint32_t count);
uint8_t DAC_GetInactiveBufferIndex(void);