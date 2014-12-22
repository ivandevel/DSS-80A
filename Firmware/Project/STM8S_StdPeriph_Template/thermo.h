#ifndef THERMO_H_
#define THERMO_H_

#define TC_K      40.0  //температурный коэффициент дл€ CJ, мк¬/∞C
#define T_RES      0.25 //дискретность представлени€ температуры CJ, ∞C (DS18B20)

#define TC_V_MIN -3 //минимальное напр€жение с термопары, м¬
#define TC_V_MAX 54 //максимальное напр€жение с термопары, м¬
#define TC_POINTS (TC_V_MAX - TC_V_MIN + 1) //количество точек таблицы


int16_t Convert(int16_t adc_code, int16_t tcj);


#endif