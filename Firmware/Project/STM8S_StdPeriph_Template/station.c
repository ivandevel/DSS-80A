#include "stm8s.h"
#include <stdlib.h>
#include "station.h" 
#include "pid.h"
#include "7-seg.h"
#include "button.h"
#include "thermo.h"

#pragma location=FLASH_DATA_START_PHYSICAL_ADDRESS 
__no_init volatile uint16_t eeSetpoint;

uint8_t prediv = 0;
uint32_t tempaccum;
uint16_t timedivider;
//uint16_t SetpointCurrent;
static uint8_t adccount=0;
static uint8_t display_setpoint=15;
uint16_t Temperature = 0;
extern uint16_t GetAdcValue(ADC1_Channel_TypeDef channel);
uint16_t Power = 0;  
uint32_t SecondTick = 0;
uint32_t usTick = 0;
uint16_t Conversion_Value = 0;
uint16_t temp = 0;
pid_t pid_s;
uint16_t Setpoint=100;
uint16_t *lcddata;
uint8_t StbyMode=FALSE;


void Soldering_TIM2_Config(void)
{
  /* Time base configuration */
  TIM2_TimeBaseInit(TIM2_PRESCALER_32, 512);

  /* PWM1 Mode configuration: Channel3 */         
  TIM2_OC3Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, 0, TIM2_OCPOLARITY_HIGH);
  TIM2_OC3PreloadConfig(ENABLE);

//    /* PWM1 Mode configuration: Channel3 */         
//  TIM2_OC2Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, 0, TIM2_OCPOLARITY_HIGH);
//  TIM2_OC2PreloadConfig(ENABLE);
  
  TIM2_ARRPreloadConfig(ENABLE);

  /* TIM2 enable counter */
  TIM2_Cmd(ENABLE);
}

void Soldering_ADC_Config (void)
{
  /*  Init GPIO for ADC2 */
  GPIO_Init(GPIOD, GPIO_PIN_3, GPIO_MODE_IN_FL_NO_IT);
  
  // Deinit ADC
ADC1_DeInit();
  
ADC1_Init(
    ADC1_CONVERSIONMODE_CONTINUOUS,
    ADC1_CHANNEL_4,
    ADC1_PRESSEL_FCPU_D4,
    ADC1_EXTTRIG_TIM, DISABLE,
    ADC1_ALIGN_RIGHT,
    ADC1_SCHMITTTRIG_CHANNEL4, DISABLE);

  ADC1_Cmd(ENABLE);
}
  

void Soldering_Main(void)
{
  //Вытаскиваем значение уставки из EEPROM
  Setpoint = eeSetpoint;
  
  pid_s.KP = 10; //8
  pid_s.KI = 22; //22
  pid_s.KD = 2; //4
  pid_s.KT = 32; //30
  
  while(1) 
  {
  if ((eButtonGetEvent(BUTTON_KEY) == eButtonEventPress)) {
      switch(StbyMode)
      {
      case MODE_WORKING:
        StbyMode = MODE_POWEROFF;
        break;
      case MODE_STANDBY:        
        StbyMode = MODE_WORKING;
        break;
      case MODE_POWEROFF:
        StbyMode = MODE_WORKING;
        break;
      }
      //if (StbyMode == MODE_WORKING) StbyMode = MODE_POWEROFF; else StbyMode = FALSE;
    }
    
    uint8_t state = ENC_GetStateEncoder();

			if (state != 0) {
                          StbyMode = MODE_WORKING;
                          
                          display_setpoint = 15;
                          
				if (state == RIGHT_SPIN) {
					Setpoint+=5;
                                        if (Setpoint >= 450) Setpoint = 450;
				}
				if (state == LEFT_SPIN) {
                                        Setpoint-=5;
                                        if (Setpoint <= 150) Setpoint = 150;
				}
			}
    
  }
}

uint16_t txa;

void Soldering_ISR (void)
{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */

   ENC_PollEncoder();

   vButtonHandler(BUTTON_KEY); 
     
   if ((StbyMode == MODE_WORKING) ||(StbyMode == MODE_STANDBY))  {
   timedivider++;
   usTick++;
   
   if (usTick == 1000)
   {
     usTick = 0;
     SecondTick++;
   }
   
   if (StbyMode == MODE_STANDBY) SecondTick = 0; 
   
   
   if (SecondTick == 60*60)
   {
     StbyMode = MODE_STANDBY;
     SecondTick = 0;
   }
     
     
   if (timedivider == 50)
   {   
    //Если задано время на отображение уставки
    if (display_setpoint)
    {
      display_setpoint--;
      //Уставку - на экран
      lcddata = &Setpoint;
      //Если кончилось время отображения значения уставки
      if (!display_setpoint) {
        //Температуру - на экран
        lcddata = &Temperature;
        //lcddata = &Power;
        eeSetpoint = Setpoint;
      }
    }
    
    ssegWriteStr("   ", 3, SEG1);
   
    switch(StbyMode)
      {
    case MODE_WORKING:
      ssegWriteInt(*lcddata);
      break;
         case MODE_STANDBY:
      ssegWriteStr("Stb", 3, SEG1);
      break; 
      }
    
   }
   
   if (timedivider == 95)
   {
     TIM2_SetCompare3(0);
   }
   
   // на 100й милисекунде запускаем измерение температуры и затем включам ШИМ
   if (timedivider > 100)
   {
    timedivider=0;
    
    tempaccum += GetAdcValue(ADC1_CHANNEL_4);

    // Усредняем значение величин с АЦП после 3х отсчетов
    if (++adccount == 5) 
    {
     //txa = Code2uV(tempaccum/5);
     //lcddata = &txa; 
      
      Temperature = Convert(tempaccum/5,1);
      //Получаем значение температуры в градусах
      //Temperature = ((uint32_t)((tempaccum/5)*42))/100 + 33;
      
      // Рассчитываем новое значение мощности
      switch(StbyMode)
      {
      case MODE_WORKING:
      Power = pid(Setpoint, Temperature, &pid_s);
      break;
      case MODE_STANDBY:
      Power = pid(Setpoint/2, Temperature, &pid_s);
      break; 
      }
      
      adccount = 0;
      tempaccum = 0;
    }
    
    TIM2_SetCompare3(Power);
    
   }
   }  else {
     TIM2_SetCompare3(0);
     ssegWriteStr("OFF", 3, SEG1);
   }
}