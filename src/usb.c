/* Includes ------------------------------------------------------------------*/

#include "usb.h"

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static USB_Clock_TypeDef USB_Clock_InitStruct;
static USB_DeviceBUSParam_TypeDef USB_DeviceBUSParam;

#ifdef USB_CDC_LINE_CODING_SUPPORTED
static USB_CDC_LineCoding_TypeDef LineCoding;
#endif /* USB_CDC_LINE_CODING_SUPPORTED */

#ifdef USB_VCOM_SYNC
    volatile uint32_t PendingDataLength = 0;
#endif /* USB_VCOM_SYNC */

/* USB protocol debugging */
#ifdef USB_DEBUG_PROTO

    #define USB_DEBUG_NUM_PACKETS   100

    typedef struct
    {
        USB_SetupPacket_TypeDef packet;
        uint32_t address;
    } TDebugInfo;

    static TDebugInfo SetupPackets[USB_DEBUG_NUM_PACKETS];
    static uint32_t SPIndex;
    static uint32_t ReceivedByteCount, SentByteCount, SkippedByteCount;

#endif /* USB_DEBUG_PROTO */

uint8_t ringBufferRx[HL_RX_BUFFER_SIZE];    /* Receive buffer (ring buffer) */
volatile uint16_t ringBufferWritePos = 0;   /*Recive buffer write position */
volatile uint16_t ringBufferReadPos = 0;    /*Recive buffer read position */

uint8_t ringBufferCache[HL_RX_BUFFER_SIZE];    /* Cache for received data */
uint8_t stringBuffer[HL_RX_BUFFER_SIZE];       /* String buffer */

static uint16_t ringBufferCacheLen = 0;
char tempString[100];

/**
  * @brief  USB Device layer setup and powering on
  * @param  None
  * @retval None
  */
void Setup_USB(void)
{
    /* Enables the CPU_CLK clock on USB */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_USB, ENABLE);

    /* Device layer initialization */
#ifdef USB_DEBUG
    USB_Clock_InitStruct.USB_USBC1_Source = USB_C1HSEdiv1;
#endif
#ifndef USB_DEBUG
    USB_Clock_InitStruct.USB_USBC1_Source = USB_C1HSEdiv2;
#endif
    USB_Clock_InitStruct.USB_PLLUSBMUL    = USB_PLLUSBMUL6;
    USB_DeviceBUSParam.MODE  = USB_SC_SCFSP_Full;
    USB_DeviceBUSParam.SPEED = USB_SC_SCFSR_12Mb;
    USB_DeviceBUSParam.PULL  = USB_HSCR_DP_PULLUP_Set;

    USB_DeviceInit(&USB_Clock_InitStruct, &USB_DeviceBUSParam);

    /* Enable all USB interrupts */
    USB_SetSIM(USB_SIS_Msk);

    USB_DevicePowerOn();

    /* Enable interrupt on USB */
#ifdef USB_INT_HANDLE_REQUIRED
    NVIC_EnableIRQ(USB_IRQn);
#endif /* USB_INT_HANDLE_REQUIRED */

    USB_DEVICE_HANDLE_RESET;
}

/**
  * @brief  Example-relating data initialization
  * @param  None
  * @retval None
  */
void VCom_Configuration(void)
{
#ifdef USB_CDC_LINE_CODING_SUPPORTED
    LineCoding.dwDTERate = 115200;
    LineCoding.bCharFormat = 0;
    LineCoding.bParityType = 0;
    LineCoding.bDataBits = 8;
#endif /* USB_CDC_LINE_CODING_SUPPORTED */
}

void USB_SendTemp(uint16_t temp){
    static uint8_t txBuffer[64];
    int len = snprintf((char*)txBuffer, sizeof(txBuffer),
                       "TEMP: %d C\r\n",temp);

    USB_CDC_SendData(txBuffer, len);
}

/**
  * @brief  USB_CDC_HANDLE_DATA_RECEIVE implementation - data echoing
  * @param  Buffer: Pointer to the user's buffer with received data
  * @param  Length: Length of data
  * @retval @ref USB_Result.
  */
USB_Result USB_CDC_RecieveData(uint8_t* Buffer, uint32_t Length)
{
    USB_Result result = USB_SUCCESS;

#ifdef USB_DEBUG_PROTO
    ReceivedByteCount += Length;
#endif /* USB_DEBUG_PROTO */

	uint16_t tempHeadPos = ringBufferWritePos;

	for (uint32_t i = 0; i < Length; i++)
	{
		uint16_t nextHeadPos = (uint16_t)((uint16_t)(tempHeadPos + 1U) % HL_RX_BUFFER_SIZE);
		if (nextHeadPos == ringBufferReadPos)
		{
			result = USB_ERROR;
			break;
		}
		ringBufferRx[tempHeadPos] = Buffer[i];
		tempHeadPos = nextHeadPos;
	}

	if (result == USB_SUCCESS)
	{
		ringBufferWritePos = tempHeadPos;
	}

#ifdef USB_DEBUG_PROTO
    if (result == USB_SUCCESS)
    {
        SentByteCount += Length;
    }
#ifndef USB_VCOM_SYNC
    else
    {
        SkippedByteCount += Length;
    }
#endif /* !USB_VCOM_SYNC */
#endif /* USB_DEBUG_PROTO */

#ifdef USB_VCOM_SYNC
    if (result != USB_SUCCESS)
    {
        /* If data cannot be sent now, it will await nearest possibility
        * (see USB_CDC_DataSent) */
        PendingDataLength = Length;
    }
    return result;
#else
    return result;
#endif /* USB_VCOM_SYNC */
}


/**
 * @brief : function clears receive buffer for USB.
 * 
 * Also function null positions:
 * - Receive buffer write position (ringBufferWritePos)
 * - Receive buffer read position (ringBufferReadPos)
 * 
 * @param  None.
 * @retval None.
 */
void USB_CDC_FlushringBufferRx_FS()
{
	memset(ringBufferRx, 0, HL_RX_BUFFER_SIZE);
	ringBufferWritePos = 0;
	ringBufferReadPos = 0;
}

#ifdef USB_VCOM_SYNC

/**
  * @brief  USB_CDC_HANDLE_DATA_SENT implementation - sending of pending data
  * @param  None
  * @retval @ref USB_Result.
  */
USB_Result USB_CDC_DataSent(void)
{
    USB_Result result = USB_SUCCESS;

    if (PendingDataLength)
    {
        result = USB_CDC_SendData(Buffer, PendingDataLength);
#ifdef USB_DEBUG_PROTO
        if (result == USB_SUCCESS)
        {
            SentByteCount += PendingDataLength;
        }
        else
        {
            SkippedByteCount += PendingDataLength;
        }
#endif /* USB_DEBUG_PROTO */
        PendingDataLength = 0;
        USB_CDC_ReceiveStart();
    }
    return USB_SUCCESS;
}

#endif /* USB_VCOM_SYNC */

#ifdef USB_CDC_LINE_CODING_SUPPORTED

/**
  * @brief  USB_CDC_HANDLE_GET_LINE_CODING implementation example
  * @param  wINDEX: Request value 2nd word (wIndex)
  * @param  DATA: Pointer to the USB_CDC Line Coding Structure
  * @retval @ref USB_Result.
  */
USB_Result USB_CDC_GetLineCoding(uint16_t wINDEX, USB_CDC_LineCoding_TypeDef* DATA)
{
    assert_param(DATA);
    
    if (wINDEX != 0)
    {
        /* Invalid interface */
        return USB_ERR_INV_REQ;
    }

    /* Just store received settings */
    *DATA = LineCoding;

    return USB_SUCCESS;
}

/**
  * @brief  USB_CDC_HANDLE_SET_LINE_CODING implementation example
  * @param  wINDEX: Request value 2nd word (wIndex)
  * @param  DATA: Pointer to the USB_CDC Line Coding Structure
  * @retval @ref USB_Result.
  */
USB_Result USB_CDC_SetLineCoding(uint16_t wINDEX, const USB_CDC_LineCoding_TypeDef* DATA)
{
    assert_param(DATA);
    
    if (wINDEX != 0)
    {
        /* Invalid interface */
        return USB_ERR_INV_REQ;
    }

    /* Just send back settings stored earlier */
    LineCoding = *DATA;

    return USB_SUCCESS;
}

#endif /* USB_CDC_LINE_CODING_SUPPORTED */

#ifdef USB_DEBUG_PROTO

/**
  * @brief  Overwritten USB_DEVICE_HANDLE_SETUP default handler - to dump received setup packets
  * @param  EPx: USB Control EndPoint (EP0) number
  * @param  USB_SetupPacket: Pointer to a USB_SetupPacket_TypeDef structure
  *         that contains received setup packet contents (on success)
  * @retval @ref USB_Result.
  */
USB_Result USB_DeviceSetupPacket_Debug(USB_EP_TypeDef EPx, const USB_SetupPacket_TypeDef* USB_SetupPacket)
{
#ifdef USB_DEBUG_PROTO
    SetupPackets[SPIndex].packet = *USB_SetupPacket;
    SetupPackets[SPIndex].address = USB_GetSA();
    SPIndex = (SPIndex < USB_DEBUG_NUM_PACKETS ? SPIndex + 1 : 0);
#endif /* USB_DEBUG_PROTO */

    return USB_DeviceSetupPacket(EPx, USB_SetupPacket);
}

#endif /* USB_DEBUG_PROTO */

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

void USB_PrintDebug(char *format, ...)
{
//#ifdef USB_DEBUG
	va_list argptr;
	va_start(argptr, format);

	vsprintf(tempString, format, argptr);
	va_end(argptr);
    USB_CDC_SendData((uint8_t *)tempString, strlen(tempString));
//#endif
}

void USB_Print(char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);

	vsprintf(tempString, format, argptr);
	va_end(argptr);
    USB_CDC_SendData((uint8_t *)tempString, strlen(tempString));
}

/**
 * 
 * @brief Processes incoming USB data and extracts a command framed by '<' and '>'.
 * 
 * This function checks the USB receive buffer for new data, identifies commands 
 * enclosed in '<...>' markers, and extracts the command into `stringBuffer`. 
 * It updates pointers to track the last found start and end markers and marks
 * the processed command as completed by replacing the markers with '='.
 * 
 * @retval !0 : A command was successfully extracted, marked as completed.
 * @retval 0  : No command was found or processed.
 */
char *extract_USB_command(void)
{
	uint16_t bytesAvailable = GetRingBufferBytesAvailable();

	if (bytesAvailable > 0U)
	{
		uint16_t freeSpace = (uint16_t)(HL_RX_BUFFER_SIZE - 1U - ringBufferCacheLen);

		if (bytesAvailable > freeSpace)
		{
			memset(ringBufferCache, 0, sizeof(ringBufferCache));
			ringBufferCacheLen = 0U;
			freeSpace = (uint16_t)(HL_RX_BUFFER_SIZE - 1U);
		}

		if (bytesAvailable > freeSpace)
		{
			bytesAvailable = freeSpace;
		}

		if ((bytesAvailable > 0U) &&
			(CopyRingBufferToBuffer(&ringBufferCache[ringBufferCacheLen], bytesAvailable) == USB_CDC_RX_BUFFER_OK))
		{
			ringBufferCacheLen = (uint16_t)(ringBufferCacheLen + bytesAvailable);
			ringBufferCache[ringBufferCacheLen] = '\0';
		}
	}

	while (ringBufferCacheLen > 0U)
	{
		char *start = strchr((char *)ringBufferCache, '<');
		char *end = strchr((char *)ringBufferCache, '>');

		if ((end != NULL) && ((start == NULL) || (end < start)))
		{
			uint16_t dropLen = (uint16_t)((end - (char *)ringBufferCache) + 1);
			memmove(ringBufferCache, end + 1, ringBufferCacheLen - dropLen);
			ringBufferCacheLen = (uint16_t)(ringBufferCacheLen - dropLen);
			ringBufferCache[ringBufferCacheLen] = '\0';
			continue;
		}

		if (start == NULL)
		{
			return NULL;
		}

		end = strchr(start + 1, '>');
		if (end == NULL)
		{
			if (start > (char *)ringBufferCache)
			{
				uint16_t keepLen = (uint16_t)(ringBufferCacheLen - (uint16_t)(start - (char *)ringBufferCache));
				memmove(ringBufferCache, start, keepLen);
				ringBufferCacheLen = keepLen;
				ringBufferCache[ringBufferCacheLen] = '\0';
			}
			return NULL;
		}

		{
			uint16_t commandLen = (uint16_t)(end - start - 1);
			uint16_t consumedLen = (uint16_t)((end - (char *)ringBufferCache) + 1);
			uint16_t remainingLen = (uint16_t)(ringBufferCacheLen - consumedLen);

			if (commandLen >= HL_RX_BUFFER_SIZE)
			{
				commandLen = (uint16_t)(HL_RX_BUFFER_SIZE - 1U);
			}

			memcpy(stringBuffer, start + 1, commandLen);
			stringBuffer[commandLen] = '\0';

			memmove(ringBufferCache, end + 1, remainingLen);
			ringBufferCacheLen = remainingLen;
			ringBufferCache[ringBufferCacheLen] = '\0';

			return (char *)stringBuffer;
		}
	}

	return NULL;
}

/**
 * @brief Clears the USB reception buffers and resets the pointers.
 * 
 * This function clears the parser cache and the RX ring buffer and resets
 * their positions, so the next USB data reception starts from a clean state.
 * 
 * @retval None
 */
void USB_Flush(void) {
    memset(ringBufferCache, 0, HL_RX_BUFFER_SIZE);
    memset(ringBufferRx, 0, HL_RX_BUFFER_SIZE);

    ringBufferCacheLen = 0;
    ringBufferWritePos = 0;
    ringBufferReadPos = 0;
}



