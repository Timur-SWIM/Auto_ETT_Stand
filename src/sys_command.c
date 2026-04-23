#include "sys_command.h"

static char tokens[MAX_TOKENS][TOKEN_LENGTH]; // usb parsing tokens pointers array
static char subtokens[SUBTOKENS_MAX][SUBTOKEN_LEN]; // subtokens array

/* =========================
 * Таблица калибровки VCO
 * ========================= */
typedef struct {
    uint16_t freq_mhz;
    uint16_t dac_code;
} VCO_Point_t;

static const VCO_Point_t vco_table[] = {
    {1500,  112},
    {1600,  298},
    {1700,  521},
    {1800,  763},
    {1900, 1030},
    {2000, 1303},
    {2100, 1613},
    {2200, 1985},
    {2300, 2606},
    {2400, 3723}
};

#define VCO_TABLE_SIZE  (sizeof(vco_table) / sizeof(vco_table[0]))

/* =========================
 * Коды возврата
 * ========================= */
typedef enum {
    GEN_OK = 0,
    GEN_ERR_NULL = -1,
    GEN_ERR_RANGE = -2,
    GEN_ERR_PARAM = -3,
    GEN_ERR_CMD = -4
} GenStatus_t;

typedef enum {
    ATT_OK = 0,
    ATT_ERR_NULL = -1,
    ATT_ERR_RANGE = -2,
    ATT_ERR_PARAM = -3,
    ATT_ERR_CMD = -4
} AttStatus_t;

static uint8_t SplitCommand(char *src, const char *sep)
{
    uint8_t count = 0;
    char *part = strtok(src, sep);

    while ((part != NULL) && (count < SUBTOKENS_MAX)) {
        strncpy(subtokens[count], part, SUBTOKEN_LEN - 1);
        subtokens[count][SUBTOKEN_LEN - 1] = '\0';
        count++;
        part = strtok(NULL, sep);
    }

    return count;
}

static int StringToU16(const char *str, uint16_t *value)
{
    char *endptr;
    unsigned long temp;

    if ((str == NULL) || (value == NULL)) {   
        return GEN_ERR_NULL;
    }

    if (*str == '\0') {
        return GEN_ERR_PARAM;
    }

    temp = strtoul(str, &endptr, 10);

    if (*endptr != '\0') {
        return GEN_ERR_PARAM;
    }

    if (temp > 65535UL) {
        return GEN_ERR_PARAM;
    }

    *value = (uint16_t)temp;
    return GEN_OK;
}

static int VCO_FreqToDac(uint16_t freq_mhz, uint16_t *dac_out)
{
    uint8_t i;

    if (dac_out == NULL) {
        return GEN_ERR_NULL;
    }

    if ((freq_mhz < vco_table[0].freq_mhz) ||
        (freq_mhz > vco_table[VCO_TABLE_SIZE - 1].freq_mhz)) {
        return GEN_ERR_RANGE;
    }

    for (i = 0; i < (VCO_TABLE_SIZE - 1U); i++) {
        uint16_t f1 = vco_table[i].freq_mhz;
        uint16_t f2 = vco_table[i + 1U].freq_mhz;
        uint16_t d1 = vco_table[i].dac_code;
        uint16_t d2 = vco_table[i + 1U].dac_code;

        if (freq_mhz == f1) {
            *dac_out = d1;
            return GEN_OK;
        }

        if ((freq_mhz > f1) && (freq_mhz <= f2)) {
            uint32_t num = (uint32_t)(freq_mhz - f1) * (uint32_t)(d2 - d1);
            uint32_t den = (uint32_t)(f2 - f1);

            *dac_out = (uint16_t)(d1 + (num / den));
            return GEN_OK;
        }
    }

    *dac_out = vco_table[VCO_TABLE_SIZE - 1U].dac_code;
    return GEN_OK;
}

static int Generator_SetSingleFreq(uint16_t freq_mhz)
{
    uint16_t dac;
    uint8_t i;

    if (VCO_FreqToDac(freq_mhz, &dac) != GEN_OK) {
        return GEN_ERR_RANGE;
    }

    for (i = 0; i < DAC_BUFFER_SIZE; i++) {
        DAC_DMA_Data[i] = dac;
    }

    Generator_ApplyBuffer();
    return GEN_OK;
}

static int Generator_SetSpan(uint16_t f_start, uint16_t f_stop)
{
    uint8_t i;

    if (f_start > f_stop) {
        return GEN_ERR_PARAM;
    }

    for (i = 0; i < DAC_BUFFER_SIZE; i++) {
        uint16_t freq;
        uint16_t dac;

        if (DAC_BUFFER_SIZE == 1U) {
            freq = f_start;
        } else {
            freq = (uint16_t)(f_start +
                   (((uint32_t)(f_stop - f_start) * i) / (DAC_BUFFER_SIZE - 1U)));
        }

        if (VCO_FreqToDac(freq, &dac) != GEN_OK) {
            return GEN_ERR_RANGE;
        }

        DAC_DMA_Data[i] = dac;
    }

    Generator_ApplyBuffer();
    return GEN_OK;
}

static int Generator_SetTable(uint8_t count, char args[][SUBTOKEN_LEN], uint8_t start_index)
{
    uint8_t i;
    uint16_t freq;
    uint16_t dac;

    if (count == 0U) {
        return GEN_ERR_PARAM;
    }

    if (count > DAC_BUFFER_SIZE) {
        return GEN_ERR_PARAM;
    }

    for (i = 0; i < count; i++) {
        if (StringToU16(args[start_index + i], &freq) != GEN_OK) {
            return GEN_ERR_PARAM;
        }

        if (VCO_FreqToDac(freq, &dac) != GEN_OK) {
            return GEN_ERR_RANGE;
        }

        DAC_DMA_Data[i] = dac;
    }

    while (i < DAC_BUFFER_SIZE) {
        DAC_DMA_Data[i] = DAC_DMA_Data[i - 1U];
        i++;
    }

    Generator_ApplyBuffer();
    return GEN_OK;
}

static int Generator_Off(void)
{
    uint8_t i;

    for (i = 0; i < DAC_BUFFER_SIZE; i++) {
        DAC_DMA_Data[i] = 0U;
    }

    Generator_ApplyBuffer();
    return GEN_OK;
}

static void Generator_ApplyBuffer(void)
{
    (void)DAC_RequestBufferUpdate(DAC_DMA_Data, DAC_BUFFER_SIZE);
}

static int Generator_Execute(uint8_t argc, char argv[][SUBTOKEN_LEN])
{
    uint16_t f1, f2;

    if (argc < 2U) {
        return GEN_ERR_CMD;
    }

    if (strcmp(argv[1], "SET") == 0) {
        if (argc != 3U) {
            return GEN_ERR_PARAM;
        }

        if (StringToU16(argv[2], &f1) != GEN_OK) {
            return GEN_ERR_PARAM;
        }

        return Generator_SetSingleFreq(f1);
    }

    if (strcmp(argv[1], "SPAN") == 0) {
        if (argc != 4U) {
            return GEN_ERR_PARAM;
        }

        if (StringToU16(argv[2], &f1) != GEN_OK) {
            return GEN_ERR_PARAM;
        }

        if (StringToU16(argv[3], &f2) != GEN_OK) {
            return GEN_ERR_PARAM;
        }

        return Generator_SetSpan(f1, f2);
    }

    if (strcmp(argv[1], "TABLE") == 0) {
        if (argc < 3U) {
            return GEN_ERR_PARAM;
        }

        return Generator_SetTable((uint8_t)(argc - 2U), argv, 2U);
    }

    if (strcmp(argv[1], "OFF") == 0) {
        if (argc != 2U) {
            return GEN_ERR_PARAM;
        }

        return Generator_Off();
    }

    return GEN_ERR_CMD;
}

static int Attenuator_Execute(uint8_t argc, char argv[][SUBTOKEN_LEN])
{
    uint16_t p_out;

    if (argc < 2U) {
        return ATT_ERR_PARAM;
    }

    if (strcmp(argv[1], "SET") == 0) {
        if (argc != 3U) {
            return ATT_ERR_PARAM;
        }
        if (StringToU16(argv[2], &p_out) != ATT_OK) {
            return ATT_ERR_PARAM;
        }
        return SetAttenuation(p_out);
    }
    if (strcmp(argv[1], "READ") == 0) {
        return ReadAttenuation();
    }
    return ATT_ERR_CMD;
}
/**
 * @brief Parses a command string into tokens.
 * 
 * @param str - command to parse
 * @retval None.
 * 
 * @ingroup command_system.c
 */
void Parse_command(char *str) {

    if (str == NULL) {
        return; // Handle null pointer input
    }

    const char separator[] = ",";    //separator in command
    uint8_t tokens_count = 0;

    char *chunk = strtok(str, separator);    // find first token

    while ((chunk != NULL) && (tokens_count < MAX_TOKENS)) {
        strncpy(tokens[tokens_count], chunk, TOKEN_LENGTH - 1); // Copy token to array, ensuring null-termination
        tokens[tokens_count][TOKEN_LENGTH - 1] = '\0';
        
        tokens_count++;
        chunk = strtok(NULL, separator);    // find next token
    }

    /* parse tokens */
    for (uint8_t i = 0; i < tokens_count; i++) {

        uint8_t subcount = SplitCommand(tokens[i], ".");

        if (subcount == 0U) {
            continue;
        }        
        switch (subtokens[0][0])
        {
            case 'G':    // Generator command
                /* code */
                int status = Generator_Execute(subcount, subtokens);
                break;
            case 'A':    // Attenuator сommand
                /* code */
                int status = Attenuator_Execute(subcount, subtokens);
                break;
            case 'D':    // Debugger command
                /* code */
                break;
            default:
                break;
        }
    }
}