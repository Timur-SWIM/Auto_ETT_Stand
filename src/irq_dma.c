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
         /*
         * PRIMARY активен, ALTERNATE неактивен.
         * DMA читает Buffer[0], писать можно в Buffer[1].
         */
        DAC_DMA_ActiveBufferIndex = 0U;

        if (DAC_DMA_UpdatePending != 0U) {
            /*
             * Новые данные уже лежат в неактивном буфере.
             * Принимаем обновление.
             */
            if (DAC_DMA_PendingBufferIndex == 1U) {
                DAC_DMA_PendingBufferIndex = 0U;
            } else {
                /*
                 * Защита от рассинхронизации состояния.
                 * Если pending-буфер почему-то не совпал,
                 * делаем оба буфера одинаковыми.
                 */
                memcpy(DAC_DMA_Buffer[1], DAC_DMA_Buffer[0], DAC_BUFFER_SIZE * sizeof(uint16_t));
                DAC_DMA_PendingBufferIndex = 0U;
            }
        }
        else
        {
            /*
             * Новых данных нет — копируем активный буфер
             * в неактивный, чтобы последовательность не "скакала".
             */
            memcpy(DAC_DMA_Buffer[1], DAC_DMA_Buffer[0], DAC_BUFFER_SIZE * sizeof(uint16_t));
        }
        /* Reconfigure the inactive DMA data structure for TIM2 */
        DMA_ChannelReloadCycle(DMA_Channel_TIM2, 
                               DMA_CTRL_DATA_ALTERNATE, 
                               DAC_BUFFER_SIZE, 
                               DMA_Mode_PingPong);
    }
    else
    {
        /*
         * ALTERNATE активен, PRIMARY неактивен.
         * DMA читает Buffer[1], писать можно в Buffer[0].
         */
        DAC_DMA_ActiveBufferIndex = 1U;

        if (DAC_DMA_UpdatePending != 0U)
        {
            if (DAC_DMA_PendingBufferIndex == 0U) {
                DAC_DMA_UpdatePending = 0U;
            }
            else
            {
                memcpy(DAC_DMA_Buffer[0], DAC_DMA_Buffer[1], DAC_BUFFER_SIZE * sizeof(uint16_t));
                DAC_DMA_UpdatePending = 0U;
            }
        }
        else
        {
            memcpy(DAC_DMA_Buffer[0], DAC_DMA_Buffer[1], DAC_BUFFER_SIZE * sizeof(uint16_t));
        }
        DMA_ChannelReloadCycle(DMA_Channel_TIM2, 
                               DMA_CTRL_DATA_PRIMARY, 
                               DAC_BUFFER_SIZE, 
                               DMA_Mode_PingPong);
    }
    /* Check if interrupt from DMA_Channel_ADC2 */
    if (DMA_GetFlagStatus(DMA_Channel_ADC2, DMA_FLAG_CHNL_ALT) == RESET)
    {
        /* Reconfigure the inactive DMA data structure for ADC2 */
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