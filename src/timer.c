#include "time.h"

void LedPortInit(void) {
    /* Enable peripheral clocks */
    RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTB, ENABLE);

    /* Reset PORTB settings */
    PORT_DeInit(LED_PORT);	
	
	/* Configure PORTB pin0 */
	PORT_StructInit(&PORT_InitStructure);
	PORT_InitStructure.PORT_Pin = LED;
	PORT_InitStructure.PORT_OE = PORT_OE_OUT;
	PORT_InitStructure.PORT_FUNC = PORT_FUNC_ALTER;
	PORT_InitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_FAST;
	
	PORT_Init(LED_PORT, &PORT_InitStructure);
}