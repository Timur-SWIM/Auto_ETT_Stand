#ifndef __PORT_H
#define __PORT_H

#include "MDR32FxQI_port.h"
#include "MDR32FxQI_rst_clk.h"

#define PORTA_PORT     MDR_PORTA
#define PORTA_PINS     (PORT_Pin_0 | PORT_Pin_1 | PORT_Pin_2 | PORT_Pin_3 | PORT_Pin_4 | PORT_Pin_5)

void PortA_Init(void);
void PortA_SetPins(uint8_t bits);

#endif /* __PORT_H */