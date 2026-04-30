#pragma once

#include "MDR32FxQI_config.h"
#include "MDR32FxQI_usb_handlers.h"
#include "MDR32FxQI_rst_clk.h"
#include "MDR32FxQI_eeprom.h"
#include "MDR32FxQI_bkp.h"
#include "port.h"
#include "ring_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

//#define USB_DEBUG

void Setup_USB(void);

void VCom_Configuration(void);

void USB_SendTemp(uint16_t temp);

USB_Result USB_CDC_RecieveData(uint8_t* Buffer, uint32_t Length);

void USB_CDC_FlushringBufferRx_FS();

USB_Result USB_CDC_GetLineCoding(uint16_t wINDEX, USB_CDC_LineCoding_TypeDef* DATA);

USB_Result USB_CDC_SetLineCoding(uint16_t wINDEX, const USB_CDC_LineCoding_TypeDef* DATA);

void USB_PrintDebug(char *format, ...);

char *extract_USB_command(void);

void USB_Flush(void);

void USB_Print(char *format, ...);
