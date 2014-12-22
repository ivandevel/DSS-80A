#ifndef THERMO_H_
#define THERMO_H_

#define TC_K      40.0  //������������� ����������� ��� CJ, ���/�C
#define T_RES      0.25 //������������ ������������� ����������� CJ, �C (DS18B20)

#define TC_V_MIN -3 //����������� ���������� � ���������, ��
#define TC_V_MAX 54 //������������ ���������� � ���������, ��
#define TC_POINTS (TC_V_MAX - TC_V_MIN + 1) //���������� ����� �������


int16_t Convert(int16_t adc_code, int16_t tcj);


#endif