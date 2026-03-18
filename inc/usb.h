#include "MDR32FxQI_config.h"
#include "MDR32FxQI_usb_handlers.h"
#include "MDR32FxQI_rst_clk.h"
#include "MDR32FxQI_eeprom.h"
#include "MDR32FxQI_bkp.h"
#include <stdio.h>

void Setup_USB(void);
void VCom_Configuration(void);
void USB_SendTemperature(float temp);
void USB_SendTemp(uint16_t temp);