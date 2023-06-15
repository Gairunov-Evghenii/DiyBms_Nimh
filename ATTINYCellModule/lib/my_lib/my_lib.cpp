#include "my_lib.h"

#define VOLTAGE_sh              (0)
#define CURRENT_sh              (1)
#define TEMPERATURE_sh          (2)
#define RESISTANCE_sh           (3)
#define DIFF_VOLTAGE_sh         (4)
#define DIFF_CURRENT_sh         (5)
#define DIFF_TEMPERATURE_sh     (6)
#define DIFF_RESISTANCE_sh      (7)

#define ERR_VOLTAGE             (0x0003<<(2*VOLTAGE_sh))
#define ERR_CURRENT             (0x0003<<(2*CURRENT_sh))
#define ERR_TEMPERATURE         (0x0003<<(2*TEMPERATURE_sh))
#define ERR_RESISTANCE          (0x0003<<(2*RESISTANCE_sh))
#define ERR_DIFF_VOLTAGE        (0x0003<<(2*DIFF_VOLTAGE_sh))
#define ERR_DIFF_CURRENT        (0x0003<<(2*DIFF_CURRENT_sh))
#define ERR_DIFF_TEMPERATURE    (0x0003<<(2*DIFF_TEMPERATURE_sh))
#define ERR_DIFF_RESISTANCE     (0x0003<<(2*DIFF_RESISTANCE_sh))

typedef enum ML_ERR_enum
{
    OK          = 0,
    OVERHIGH    = 1,
    UNDERLOW    = 2,
}   ML_ERROR;
