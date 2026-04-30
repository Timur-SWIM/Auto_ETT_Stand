#include "irq_dma.h"

/**
  * @brief  Unified DMA interrupt handler.
  *         Handles interrupts from DMA channels used in the project:
  *         - DMA_Channel_TIM2 (for DAC)
  *         - DMA_Channel_ADC2 (for ADC)
  * @param  None
  * @retval None
  */
void DMA_IRQHandler(void)
{
    static uint8_t tim2_alt_prev = 0U;
    static uint8_t adc2_alt_prev = 0U;
    uint8_t tim2_alt_now;
    uint8_t adc2_alt_now;

    tim2_alt_now = (uint8_t)DMA_GetFlagStatus(DMA_Channel_TIM2, DMA_FLAG_CHNL_ALT);
    adc2_alt_now = (uint8_t)DMA_GetFlagStatus(DMA_Channel_ADC2, DMA_FLAG_CHNL_ALT);

    if (tim2_alt_now != tim2_alt_prev)
    {
        tim2_alt_prev = tim2_alt_now;

        if (tim2_alt_now == RESET)
        {
            DAC_DMA_ActiveBufferIndex = 0U;

            if (DAC_DMA_UpdatePending != 0U) {
                if (DAC_DMA_PendingBufferIndex == 1U) {
                    DAC_DMA_PendingBufferIndex = 0U;
                } else {
                    memcpy(DAC_DMA_Buffer[1], DAC_DMA_Buffer[0], DAC_BUFFER_SIZE * sizeof(uint16_t));
                    DAC_DMA_PendingBufferIndex = 0U;
                }
            } else {
                memcpy(DAC_DMA_Buffer[1], DAC_DMA_Buffer[0], DAC_BUFFER_SIZE * sizeof(uint16_t));
            }

            DMA_ChannelReloadCycle(DMA_Channel_TIM2,
                                   DMA_CTRL_DATA_ALTERNATE,
                                   DAC_BUFFER_SIZE,
                                   DMA_Mode_PingPong);
        }
        else
        {
            DAC_DMA_ActiveBufferIndex = 1U;

            if (DAC_DMA_UpdatePending != 0U) {
                if (DAC_DMA_PendingBufferIndex == 0U) {
                    DAC_DMA_UpdatePending = 0U;
                } else {
                    memcpy(DAC_DMA_Buffer[0], DAC_DMA_Buffer[1], DAC_BUFFER_SIZE * sizeof(uint16_t));
                    DAC_DMA_UpdatePending = 0U;
                }
            } else {
                memcpy(DAC_DMA_Buffer[0], DAC_DMA_Buffer[1], DAC_BUFFER_SIZE * sizeof(uint16_t));
            }

            DMA_ChannelReloadCycle(DMA_Channel_TIM2,
                                   DMA_CTRL_DATA_PRIMARY,
                                   DAC_BUFFER_SIZE,
                                   DMA_Mode_PingPong);
        }
    }

    if (adc2_alt_now != adc2_alt_prev)
    {
        adc2_alt_prev = adc2_alt_now;

        if (adc2_alt_now == RESET)
        {
            DMA_ChannelReloadCycle(DMA_Channel_ADC2,
                                   DMA_CTRL_DATA_ALTERNATE,
                                   DAC_BUFFER_SIZE,
                                   DMA_Mode_PingPong);
        }
        else
        {
            DMA_ChannelReloadCycle(DMA_Channel_ADC2,
                                   DMA_CTRL_DATA_PRIMARY,
                                   DAC_BUFFER_SIZE,
                                   DMA_Mode_PingPong);
        }
    }
}
