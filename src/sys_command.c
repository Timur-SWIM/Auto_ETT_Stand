#include "sys_command.h"

static char tokens[MAX_TOKENS][TOKEN_LENGTH]; // usb parsing tokens pointers array
static char subtokens[SUBTOKENS_MAX][SUBTOKEN_LEN]; // subtokens array

/* =========================
 * Таблица калибровки VCO
 * ========================= */
static const VCO_Point_t vco_table[] = {
    {1500,  112, 320},
    {1600,  298, 324},
    {1700,  521, 323},
    {1800,  763, 326},
    {1900, 1030, 341},
    {2000, 1303, 355},
    {2100, 1613, 380},
    {2200, 1985, 363},
    {2300, 2606, 341},
    {2400, 3723, 371}
};

/* =========================
 * Коды возврата
 * ========================= */
typedef enum {
    OK = 0,
    ERR_NULL = -1,
    ERR_RANGE = -2,
    ERR_PARAM = -3,
    ERR_CMD = -4
} Status_t;

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
        return ERR_NULL;
    }

    if (*str == '\0') {
        return ERR_PARAM;
    }

    temp = strtoul(str, &endptr, 10);

    if (*endptr != '\0') {
        return ERR_PARAM;
    }

    if (temp > 65535UL) {
        return ERR_PARAM;
    }

    *value = (uint16_t)temp;
    return OK;
}

static int VCO_FreqToDac(uint16_t freq_mhz, uint16_t *dac_out)
{
    uint8_t i;

    if (dac_out == NULL) {
        return ERR_NULL;
    }

    if ((freq_mhz < vco_table[0].freq_mhz) ||
        (freq_mhz > vco_table[VCO_TABLE_SIZE - 1].freq_mhz)) {
        return ERR_RANGE;
    }

    for (i = 0; i < (VCO_TABLE_SIZE - 1U); i++) {
        uint16_t f1 = vco_table[i].freq_mhz;
        uint16_t f2 = vco_table[i + 1U].freq_mhz;
        uint16_t d1 = vco_table[i].dac_code;
        uint16_t d2 = vco_table[i + 1U].dac_code;

        if (freq_mhz == f1) {
            *dac_out = d1;
            return OK;
        }

        if ((freq_mhz > f1) && (freq_mhz <= f2)) {
            uint32_t num = (uint32_t)(freq_mhz - f1) * (uint32_t)(d2 - d1);
            uint32_t den = (uint32_t)(f2 - f1);

            *dac_out = (uint16_t)(d1 + (num / den));
            return OK;
        }
    }

    *dac_out = vco_table[VCO_TABLE_SIZE - 1U].dac_code;
    return OK;
}

static int Generator_SetSingleFreq(uint16_t freq_mhz)
{
    uint16_t dac;
    uint8_t i;

    if (VCO_FreqToDac(freq_mhz, &dac) != OK) {
        return ERR_RANGE;
    }

    for (i = 0; i < DAC_BUFFER_SIZE; i++) {
        DAC_DMA_Data[i] = dac;
    }

    Generator_ApplyBuffer();
        return OK;
}

static int Generator_SetSpan(uint16_t f_start, uint16_t f_stop)
{
    uint8_t i;

    if (f_start > f_stop) {
        return ERR_PARAM;
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

        if (VCO_FreqToDac(freq, &dac) != OK) {
            return ERR_RANGE;
        }

        DAC_DMA_Data[i] = dac;
    }

    Generator_ApplyBuffer();
    return OK;
}

static int Generator_SetTable(uint8_t count, char args[][SUBTOKEN_LEN], uint8_t start_index)
{
    uint8_t i;
    uint16_t freq;
    uint16_t dac;

    if (count == 0U) {
        return ERR_PARAM;
    }

    if (count > DAC_BUFFER_SIZE) {
        return ERR_PARAM;
    }

    for (i = 0; i < count; i++) {
        if (StringToU16(args[start_index + i], &freq) != OK) {
            return ERR_PARAM;
        }

        if (VCO_FreqToDac(freq, &dac) != OK) {
            return ERR_RANGE;
        }

        DAC_DMA_Data[i] = dac;
    }

    while (i < DAC_BUFFER_SIZE) {
        DAC_DMA_Data[i] = DAC_DMA_Data[i - 1U];
        i++;
    }

    Generator_ApplyBuffer();
    return OK;
}

int Generator_Off(void)
{
    uint8_t i;

    for (i = 0; i < DAC_BUFFER_SIZE; i++) {
        DAC_DMA_Data[i] = 0U;
    }

    Generator_ApplyBuffer();
    return OK;
}

static void Generator_ApplyBuffer(void)
{
    (void)DAC_RequestBufferUpdate(DAC_DMA_Data, DAC_BUFFER_SIZE);
}

static int Generator_Execute(uint8_t argc, char argv[][SUBTOKEN_LEN])
{
    uint16_t f1, f2;

    if (argc < 2U) {
        return ERR_CMD;
    }

    if (strcmp(argv[1], "SET") == 0) {
        if (argc != 3U) {
            return ERR_PARAM;
        }

        if (StringToU16(argv[2], &f1) != OK) {
            return ERR_PARAM;
        }

        return Generator_SetSingleFreq(f1);
    }

    if (strcmp(argv[1], "SPAN") == 0) {
        if (argc != 4U) {
            return ERR_PARAM;
        }

        if (StringToU16(argv[2], &f1) != OK) {
            return ERR_PARAM;
        }

        if (StringToU16(argv[3], &f2) != OK) {
            return ERR_PARAM;
        }

        return Generator_SetSpan(f1, f2);
    }

    if (strcmp(argv[1], "TABLE") == 0) {
        if (argc < 3U) {
            return ERR_PARAM;
        }

        return Generator_SetTable((uint8_t)(argc - 2U), argv, 2U);
    }

    if (strcmp(argv[1], "OFF") == 0) {
        if (argc != 2U) {
            return ERR_PARAM;
        }

        return Generator_Off();
    }

    return ERR_CMD;
}

int ReadAttenuation(void)
{
    return OK;
}

static int find_index_in_vco_table(uint16_t dac_code)
{
    for (uint8_t i = 0; i < VCO_TABLE_SIZE; i++) {
        if (vco_table[i].dac_code == dac_code) {
            return (int)i;
        }
    }
    return ERR_PARAM;
}

static uint16_t find_PoutMax_in_VCO_table(uint8_t start_index, uint8_t end_index)
{
    uint16_t p_max = 0;
    for (uint8_t i = start_index; i <= end_index; i++) {
        if (vco_table[i].p_out_dbm > p_max) {
            p_max = vco_table[i].p_out_dbm;
        }
    }

    return p_max;
}

int SetAttenuation(uint16_t p_out)
{
    if (p_out > 315U || p_out < 0U) {
        return ERR_PARAM;
    }
    uint16_t p_buf;
    uint16_t a_req;
    uint8_t inactive_Buffer = DAC_GetInactiveBufferIndex();
    uint16_t d_min = DAC_DMA_Buffer[inactive_Buffer][0];
    uint16_t d_max = DAC_DMA_Buffer[inactive_Buffer][DAC_BUFFER_SIZE - 1U];
    USB_PrintDebug("Requested attenuation for P_out %d dBm. Inactive buffer DAC range: %d - %d\r\n", p_out, d_min, d_max);
    if (d_max == d_min) {
        int p_buf_index = find_index_in_vco_table(d_min);
        if (p_buf_index == ERR_PARAM) {
            return ERR_PARAM;
        }
        p_buf = vco_table[p_buf_index].p_out_dbm;
    } else {
        int d_min_index = find_index_in_vco_table(d_min);
        int d_max_index = find_index_in_vco_table(d_max);
        USB_PrintDebug("DAC range corresponds to VCO table indices: %d - %d\r\n", d_min_index, d_max_index);

        if (d_min_index == ERR_PARAM || d_max_index == ERR_PARAM) {
            return ERR_PARAM;
        }
        p_buf = find_PoutMax_in_VCO_table((uint8_t)d_min_index, (uint8_t)d_max_index);
    }
    a_req = p_buf - p_out;
    if (a_req < 0) {
        a_req = 0;
    }
    if (a_req > 300) {
        a_req = 300;
    }
    uint16_t att_steps  = (a_req / 10) * 2;
    uint8_t att_code = att_steps;

    if (att_code > 60) { 
        att_code = 60;
    }

    PortA_SetPins(att_code);
    USB_PrintDebug("Attenuation set to P_buf(%d)dB - P_set(%d)dB = A_set %d dB (code: %d)\r\n", p_buf, p_out, a_req, att_code);

    return OK;
}

static int Attenuator_Execute(uint8_t argc, char argv[][SUBTOKEN_LEN])
{
    uint16_t p_out;

    if (argc < 2U) {
        return ERR_PARAM;
    }

    if (strcmp(argv[1], "SET") == 0) {
        if (argc != 3U) {
            return ERR_PARAM;
        }
        if (StringToU16(argv[2], &p_out) != OK) {
            return ERR_PARAM;
        }
        return SetAttenuation(p_out);
    }
    if (strcmp(argv[1], "READ") == 0) {
        return ReadAttenuation();
    }
    return ERR_CMD;
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
                (void)Generator_Execute(subcount, subtokens);
                break;
            case 'A':    // Attenuator сommand
                (void)Attenuator_Execute(subcount, subtokens);
                break;
            case 'D':    // Debugger command
                /* code */
                break;
            case 'I':    // Information command
                USB_Print("MDR32_ETT_Stand_Board\r\n");
                break;
            default:
                break;
        }
    }
}
