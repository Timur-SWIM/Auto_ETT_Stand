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
    /* Check if interrupt from DMA_Channel_TIM2 (DAC) */

    if (DMA_GetFlagStatus(DMA_Channel_TIM2, DMA_FLAG_CHNL_ALT) == RESET)
    {
        /* Reconfigure the inactive DMA data structure for TIM2 */
        DMA_ChannelReloadCycle(DMA_Channel_TIM2, DMA_CTRL_DATA_ALTERNATE, 10, DMA_Mode_PingPong);
    }
    else
    {
        DMA_ChannelReloadCycle(DMA_Channel_TIM2, DMA_CTRL_DATA_PRIMARY, 10, DMA_Mode_PingPong);
    }

    /* Check if interrupt from DMA_Channel_ADC2 */
    if (DMA_GetFlagStatus(DMA_Channel_ADC2, DMA_FLAG_CHNL_ALT) == RESET)
    {
        /* Reconfigure the inactive DMA data structure for ADC2 */
        DMA_ChannelReloadCycle(DMA_Channel_ADC2, DMA_CTRL_DATA_ALTERNATE, 10, DMA_Mode_PingPong);
    }
    else
    {
        DMA_ChannelReloadCycle(DMA_Channel_ADC2, DMA_CTRL_DATA_PRIMARY, 10, DMA_Mode_PingPong);
    }

}