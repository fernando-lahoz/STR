#include "HTS221.h"
#include "util.h"

#define DEV_WRITE 0
#define DEV_READ  1

#define DEV_ADDR    (0xBE)
#define DEV_ID      (0xBC)

#define ADDR_BURST (1 << 7) // Set this bit in reg addr if multiple registers are being read/written

#define ADDR_WHO_AM_I       (0x0F)
#define ADDR_AVG_CONF       (0x10)
#define ADDR_CTRL1          (0x20)
#define ADDR_CTRL2          (0x21)
#define ADDR_CTRL3          (0x22)
#define ADDR_STATUS         (0x27)
#define ADDR_OUTPUT         (0x28)
#define ADDR_CALIBRATION    (0X30)

struct avg_conf {
    uint8_t avg_hum:3; // SAMPLES = 2^(avg+1)
    uint8_t avg_temp:3; // SAMPLES = 2^(avg+2)
    uint8_t _reserved:2;
};

struct ctrl_reg
{
    struct ctrl_reg1 {
        uint8_t output_data_rate:2; // 00 -> one-shot; 01 -> 1Hz; 10 -> 7Hz; 11 -> 12.5Hz
        uint8_t block_data_update:1; // 0 -> continuous; 1 -> wait until receive LSB and MSB
        uint8_t _reserved1:4;
        uint8_t powerdown:1; // 0 -> off; 1 -> on
    } reg1;

    struct ctrl_reg2 {
        uint8_t one_shot_en:1; // 0 -> doing nothing; 1 -> start conversion (auto clears)
        uint8_t heater_en:1;
        uint8_t _reserved2:5;
        uint8_t boot:1; // clear memory
    } reg2;

    struct ctrl_reg3 {
        uint8_t _reserved3:2;
        uint8_t drdy_en:1; // 1 -> enables drdy signal
        uint8_t _reserved4:3;
        uint8_t pp_op:1; // 0 -> push-pull; 1 -> open_drain
        uint8_t drdy_polarity:1; // 0 -> active high; 1 -> active low
    } reg3;
};

struct status_reg {
    uint8_t t_data_available:1;
    uint8_t h_data_available:1;
    uint8_t _reserved:6;
};

struct output {
    int16_t H;
    int16_t T;
};

struct calibration {
    uint8_t H0_rH_x2;
    uint8_t H1_rH_x2;
    uint8_t T0_degC_x8_low;
    uint8_t T1_degC_x8_low;
    uint8_t _reserved1;
    struct {
        uint8_t T0_degC_x8_high:2;
        uint8_t T1_degC_x8_high:2;
        uint8_t _reserved2:4;
    };
    int16_t H0;
    uint16_t _reserved3;
    int16_t H1;
    int16_t T0;
    int16_t T1;
};


static struct ctrl_reg ctrl_reg = {};
static struct avg_conf avg_conf = {};

static struct calibration calibration = {};

#define AWAIT_RX_IT osEventFlagsWait(event_flagsHandle, EVENT_FLAG_I2C_MEM_RX_DONE, osFlagsWaitAll, osWaitForever)
#define AWAIT_TX_IT osEventFlagsWait(event_flagsHandle, EVENT_FLAG_I2C_MEM_TX_DONE, osFlagsWaitAll, osWaitForever)

int HTS221_check_dev()
{

	uint8_t dev_id = 0;
    HAL_I2C_Mem_Read_IT(&hi2c2, DEV_ADDR | DEV_READ, ADDR_WHO_AM_I, 1,
    		(uint8_t*)&dev_id, sizeof(uint8_t));
    AWAIT_RX_IT;
    return dev_id == DEV_ID;
}

void HTS221_configure(enum HTS221_Temp_Avg avg_samples_temp,
        enum HTS221_Hum_Avg avg_samples_hum, enum HTS221_ODR output_data_rate)
{
    ctrl_reg.reg1.output_data_rate = output_data_rate;
    ctrl_reg.reg1.block_data_update = 1;
    ctrl_reg.reg1.powerdown = 0; // power off
    ctrl_reg.reg2.one_shot_en = 0;
    ctrl_reg.reg2.heater_en = 0;
    ctrl_reg.reg2.boot = 1;
    ctrl_reg.reg3.drdy_en = 1;
    ctrl_reg.reg3.pp_op = 0; // Push-Pull
    ctrl_reg.reg3.drdy_polarity = 0; // Active high
    HAL_I2C_Mem_Write_IT(&hi2c2, DEV_ADDR | DEV_WRITE, ADDR_BURST | ADDR_CTRL1, 1,
            (uint8_t*)&ctrl_reg, sizeof(ctrl_reg));
    AWAIT_TX_IT;

	avg_conf.avg_hum = avg_samples_hum;
    avg_conf.avg_temp = avg_samples_temp;
    HAL_I2C_Mem_Write_IT(&hi2c2, DEV_ADDR | DEV_WRITE, ADDR_AVG_CONF, 1,
    		(uint8_t*)&avg_conf, sizeof(avg_conf));
    AWAIT_TX_IT;
}

void HTS221_start()
{
    ctrl_reg.reg1.powerdown = 1;
    HAL_I2C_Mem_Write_IT(&hi2c2, DEV_ADDR | DEV_WRITE, ADDR_CTRL1, 1,
    		(uint8_t*)&ctrl_reg.reg1, sizeof(ctrl_reg.reg1));
    AWAIT_TX_IT;

    HAL_I2C_Mem_Read_IT(&hi2c2, DEV_ADDR | DEV_READ, ADDR_BURST | ADDR_CALIBRATION, 1,
    		(uint8_t*)&calibration, sizeof(calibration));
    AWAIT_RX_IT;
}

void HTS221_sample()
{
    ctrl_reg.reg2.one_shot_en = 1;
    ctrl_reg.reg2.heater_en = 0;
	ctrl_reg.reg2.boot = 0;
    HAL_I2C_Mem_Write_IT(&hi2c2, DEV_ADDR | DEV_WRITE, ADDR_CTRL2, 1,
    		(uint8_t*)&(ctrl_reg.reg2), sizeof(ctrl_reg.reg2));
    AWAIT_TX_IT;
}

int HTS221_is_ready()
{
	struct status_reg status;
    HAL_I2C_Mem_Read_IT(&hi2c2, DEV_ADDR | DEV_READ, ADDR_STATUS, 1,
    		(uint8_t*)&status, sizeof(status));
    AWAIT_RX_IT;
    return status.h_data_available;
}

void HTS221_get_data(int16_t *temperature_x8, int16_t *humidity_x2)
{
    int16_t num, den;
    int16_t t0_degC_x8, t1_degC_x8;
    struct output output;

    HAL_I2C_Mem_Read_IT(&hi2c2, DEV_ADDR | DEV_READ, ADDR_BURST | ADDR_OUTPUT, 1,
    		(uint8_t*)&output, sizeof(output));
    AWAIT_RX_IT;

    // Linear Interpolation
    num = (output.H - calibration.H0) * (calibration.H1_rH_x2 - calibration.H0_rH_x2);
    den = (calibration.H1 - calibration.H0);
    *humidity_x2 = (num / den) + calibration.H0_rH_x2;

    t1_degC_x8 = ((uint16_t)(calibration.T1_degC_x8_high) << 8) | calibration.T1_degC_x8_low;
    t0_degC_x8 = ((uint16_t)(calibration.T0_degC_x8_high) << 8) | calibration.T0_degC_x8_low;
    num = (output.T - calibration.T0) * (t1_degC_x8 - t0_degC_x8);
    den = (calibration.T1 - calibration.T0);
    *temperature_x8 = (num / den) + t0_degC_x8;
}
