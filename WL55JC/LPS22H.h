#ifndef LPS22H_H
#define LPS22H_H


int LPS22H_check_dev();
void LPS22H_config(unit8_t odr);
int32_t getPressure();

#endif