#pragma once

#include <stdlib.h>
#include <stdint.h>
//#include "usb.h"

#define HL_RX_BUFFER_SIZE 256

typedef enum
{
	USB_CDC_RX_BUFFER_OK = 0U,
	USB_CDC_RX_BUFFER_NO_DATA
} USB_CDC_RX_BUFFER_StatusTypeDef;

uint16_t GetRingBufferBytesAvailable(void);

uint8_t CopyRingBufferToBuffer(uint8_t *Buf, uint16_t Len);

uint8_t CopyBufferToRingBuffer(uint8_t *Buf, uint16_t Len);

void copyRingBufferExcludingBounds(const char *buffer, int bufferSize, const char *start, const char *end, char *dest);

char* strchr_from_ring_buffer(const char *buffer, size_t bufferSize, const char *start, char c);
