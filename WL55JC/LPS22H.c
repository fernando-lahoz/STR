#include "stm32wlxx_hal.h"
#include "LPS22H.h"

#define DEV_RD              0
#define DEV_WR              1
#define SENSITIVITY         4096

//Direccion del dispositivo
#define SB28_ADD            0xBA

//Registros del dispositivo
#define WHO_AM_I            0x0F
#define CTRL_REG1           0x11
#define CTRL_REG2           0x12
#define STATUS              0x27
#define PRESSURE_OUT_XL     0x28
#define PRESSURE_OUT_L      0x29
#define PRESSURE_OUT_H      0x2A

uint8_t press_out_h, press_out_l, press_out_xl;
int32_t pressureValue;


struct ctrl_reg
{
    struct reg1_t
    {
        uint8_t odr20:3;//Los Hz que puede ir: oneshot, 1Hz, 10Hz, 25Hz, 50Hz, 75Hz, 100Hz, 200Hz
        uint8_t en_lpfp;//Enable Low pass filter: 0 disable 1:enable
        uint8_t lpfp_cfg;//Low pass filter default 0
        uint8_t bdu;//Block data update default 0: 0 continous 1 output registers not updated until MSB and LSB have been read
        uint8_t sim; //Serial Interface mode default 0: 0 4-wire 1 3-wire
    }reg1;

    struct reg2_t
    {
        uint8_t boot;//Reboot de memoria. Deafult 0: 0 normal mode 1 reboot memory content
        uint8_t int_h_l;//interrut active-high, active-low. Default 0: 0 push-pull 1 open drain
        uint8_t pp_od;//Push-pull/open-drain. Default 0: 0 PP 1 OD
        uint8_t if_add_inc;//Register address automatically incremneted during a multiple byte acces. Default 1: 0 disable 1 enable
        uint8_t nada = 0;
        uint8_t swreset;//Software reset. Default 0: 0 normal mode 1 software reset. Selfcleared when finish
        uint8_t low_noise_en;//Enable low noise used only if odr < 100. Default 0: 0 low-current mode 1 low-noise mode
        uint8_t one_shot;//Enablesone-shot Default 0: 0 idle mode 1 new dataset is adquired
    }reg2;
};

static struct ctrl_reg ctrl;

void config_reg1(uint8_t odr){
    ctrl.reg1.odr20 = odr;
    ctrl.reg1.en_lpfp = 0;
    ctrl.reg1.lpfp_cfg = 0;
    ctrl.reg1.bdu = 0;//Defautl 0, if the are problems with the output put 1
    ctrl.reg1.sim = 0;
}

void config_reg2(uint8_t odr){
    ctrl.reg2.boot = 0;
    ctrl.reg2.int_h_l = 0;
    ctrl.reg2.pp_od = 0;
    ctrl.reg2.if_add_inc = 1;
    ctrl.reg2.swreset = 0;
    ctrl.reg2.low_noise.en = 0;
    ctrl.reg2.one_shot = 0;
}

int LPS22H_check_dev(){
    unit8_t dev_id = 0;
    HAL_I2C_Mem_Read(hi2c1, SB28_ADD, WHO_AM_I | DEV_RD, sizeof(uint8_t), &dev_id, osWaitForever);
    return dev_id == SB28_ADD;
}

void LPS22H_config(unit8_t odr){
    config_reg1(odr);
    config_reg2();
    HAL_I2C_Mem_Write(hi2c1, SB28_ADD, WHO_AM_I | DEV_WR, sizeof(ctrl) &ctrl, osWaitForever);
}

int32_t getPressure(){
    HAL_I2C_Mem_Read(hi2c1, SB28_ADD, PRESSURE_OUT_H | DEV_RD, sizeof(uint8_t), &press_out_h, osWaitForever);
    HAL_I2C_Mem_Read(hi2c1, SB28_ADD, PRESSURE_OUT_L | DEV_RD, sizeof(uint8_t), &press_out_l, osWaitForever);
    HAL_I2C_Mem_Read(hi2c1, SB28_ADD, PRESSURE_OUT_XL | DEV_RD, sizeof(uint8_t), &press_out_xl, osWaitForever);


    pressureValue = (press_out_h << 16) + (press_out_l << 8) + (press_out_xl);
    
    return pressureValue / SENSITIVITY; 
    
}