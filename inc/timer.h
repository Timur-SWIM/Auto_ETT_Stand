#include "MDR32FxQI_port.h"
#include "MDR32FxQI_timer.h"
#include "MDR32FxQI_rst_clk.h"

#define LED_PORT     MDR_PORTB
#define LED         PORT_Pin_7

void LedPortInit(void);
void Timer_1_Init(void);
void TIMER1_IRQHandler(void);

extern volatile uint8_t usb_transmit_flag;