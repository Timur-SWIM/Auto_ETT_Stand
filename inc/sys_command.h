#ifndef SYS_COMMAND_H
#define SYS_COMMAND_H
#include "dac.h"
#include "port.h"
#include "usb.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define BUFFER_LENGTH 128
#define MAX_TOKENS 5
#define TOKEN_LENGTH (BUFFER_LENGTH * 2)

#define SUBTOKENS_MAX   16
#define SUBTOKEN_LEN    32

typedef struct {
    uint16_t freq_mhz;
    uint16_t dac_code;
    uint16_t p_out_dbm;
} VCO_Point_t;

#define VCO_TABLE_SIZE  (sizeof(vco_table) / sizeof(vco_table[0]))

typedef struct {
    uint16_t freq_mhz;
    float p_out_dbm;
} dac_pout_point_t;

#define DAC_POUT_TABLE_SIZE (sizeof(g_dac_pout_table) / sizeof(g_dac_pout_table[0]))

static uint8_t SplitCommand(char *src, const char *sep);
static int StringToU16(const char *str, uint16_t *value);
static int VCO_FreqToDac(uint16_t freq_mhz, uint16_t *dac_out);

static int Generator_SetSingleFreq(uint16_t freq_mhz);
static int Generator_SetSpan(uint16_t f_start, uint16_t f_stop);
static int Generator_SetTable(uint8_t count, char args[][SUBTOKEN_LEN], uint8_t start_index);
static int Generator_Off(void);

static int Generator_Execute(uint8_t argc, char argv[][SUBTOKEN_LEN]);
static void Generator_ApplyBuffer(void);

void Parse_command(char *str);
#endif /* SYS_COMMAND_H */