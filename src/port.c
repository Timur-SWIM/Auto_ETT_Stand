#include "port.h"

void PortA_Init(void)
{
    /* Enable peripheral clock for PORTA */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTA, ENABLE);

    /* Reset PORTA settings */
    PORT_DeInit(PORTA_PORT);

    /* Configure pins 0-5 as digital outputs */
    PORT_InitTypeDef PORT_InitStructure;
    PORT_StructInit(&PORT_InitStructure);
    PORT_InitStructure.PORT_Pin = PORTA_PINS;
    PORT_InitStructure.PORT_OE = PORT_OE_OUT;
    PORT_InitStructure.PORT_FUNC = PORT_FUNC_PORT;
    PORT_InitStructure.PORT_MODE = PORT_MODE_DIGITAL;
    PORT_InitStructure.PORT_SPEED = PORT_SPEED_FAST;

    PORT_Init(PORTA_PORT, &PORT_InitStructure);

    PORT_SetBits(PORTA_PORT, PORTA_PINS);
}

void PortA_SetPins(uint8_t bits)
{
    /* bits: bit0 -> PA0, bit1 -> PA1, ... bit5 -> PA5 */
    bits &= 0x3F; /* Ensure only lower 6 bits are used */
    uint32_t pins = 0;

    if (bits & 0x01) pins |= PORT_Pin_0;
    if (bits & 0x02) pins |= PORT_Pin_1;
    if (bits & 0x04) pins |= PORT_Pin_2;
    if (bits & 0x08) pins |= PORT_Pin_3;
    if (bits & 0x10) pins |= PORT_Pin_4;
    if (bits & 0x20) pins |= PORT_Pin_5;

    /* Set pins high according to bits, clear others */
    PORT_SetBits(PORTA_PORT, pins);
    PORT_ResetBits(PORTA_PORT, PORTA_PINS & ~pins);
}
