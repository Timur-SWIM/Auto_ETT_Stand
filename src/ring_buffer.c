#include "ring_buffer.h"

extern uint8_t ringBufferRx[];
extern volatile uint16_t ringBufferWritePos;
extern volatile uint16_t ringBufferReadPos;

uint16_t GetRingBufferBytesAvailable(void)
{
    int diff = (int)ringBufferWritePos - (int)ringBufferReadPos;

    if (diff >= 0)
    {
        return (uint16_t)diff;
    }

    return (uint16_t)(HL_RX_BUFFER_SIZE - (uint16_t)(-diff));
}

uint8_t CopyBufferToRingBuffer(uint8_t *Buf, uint16_t Len)
{
    uint16_t tempHeadPos = ringBufferWritePos;

    for (uint16_t i = 0; i < Len; i++)
    {
        uint16_t nextHeadPos = (uint16_t)((uint16_t)(tempHeadPos + 1U) % HL_RX_BUFFER_SIZE);

        if (nextHeadPos == ringBufferReadPos)
        {
            return USB_CDC_RX_BUFFER_NO_DATA;
        }

        ringBufferRx[tempHeadPos] = Buf[i];
        tempHeadPos = nextHeadPos;
    }

    ringBufferWritePos = tempHeadPos;
    return USB_CDC_RX_BUFFER_OK;
}

uint8_t CopyRingBufferToBuffer(uint8_t *Buf, uint16_t Len)
{
    uint16_t bytesAvailable = GetRingBufferBytesAvailable();

    if (bytesAvailable < Len)
    {
        return USB_CDC_RX_BUFFER_NO_DATA;
    }

    for (uint16_t i = 0; i < Len; i++)
    {
        Buf[i] = ringBufferRx[ringBufferReadPos];
        ringBufferReadPos =
            (uint16_t)((uint16_t)(ringBufferReadPos + 1U) % HL_RX_BUFFER_SIZE);
    }

    return USB_CDC_RX_BUFFER_OK;
}

void copyRingBufferExcludingBounds(const char *buffer, int bufferSize, const char *start, const char *end, char *dest)
{
    const char *current = start + 1;

    while (current != end)
    {
        *dest++ = *current;
        current++;

        if (current >= buffer + bufferSize)
        {
            current = buffer;
        }
    }

    *dest = '\0';
}

char* strchr_from_ring_buffer(const char *buffer, size_t bufferSize, const char *start, char c)
{
    const char *current = start;

    for (size_t i = 0; i < bufferSize; i++)
    {
        if (*current == c)
        {
            return (char *)current;
        }

        current++;
        if (current >= buffer + bufferSize)
        {
            current = buffer;
        }
    }

    return NULL;
}
