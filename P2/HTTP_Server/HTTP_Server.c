#include <stdio.h>
#include "main.h"
#include "rl_net.h"
#include "stm32f4xx_hal.h"
#include "leds.h"
#include "lcd.h"
#include "adc.h"
#include "rtc.h"

#define APP_MAIN_STK_SZ (1024U)
uint64_t app_main_stk[APP_MAIN_STK_SZ / 8];
const osThreadAttr_t app_main_attr = {
  .stack_mem  = &app_main_stk[0],
  .stack_size = sizeof(app_main_stk)
};

extern osThreadId_t TID_Display;
extern osThreadId_t TID_Led;
osThreadId_t TID_Display;
osThreadId_t TID_Led;
osThreadId_t TID_RTC;

bool LEDrun;
char lcd_text[2][20+1];
int i;
ADC_HandleTypeDef adchandle;
float value = 0;

uint8_t aShowTime[50] = {0};
uint8_t aShowDate[50] = {0};

__NO_RETURN void app_main (void *arg);
static void BlinkLed  (void *arg);
static void Display   (void *arg);
static void ThreadRTC (void *arg);

uint16_t AD_in (uint32_t ch) {
  int32_t val = 0;
  if (ch == 0) {
    val = ADC_getValue(ADC_CHANNEL_10);
  }
  return ((uint16_t)val);
}

uint8_t get_button (void) {
  return 0;
}

void netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len) {
}

/*----------------------------------------------------------------------------
  Display: gestiona texto enviado desde la web al LCD
  Se mantiene para compatibilidad con lcd.cgi del servidor web.
  El flag 0x01 lo manda HTTP_Server_CGI cuando llegan datos por POST.
 *---------------------------------------------------------------------------*/
static __NO_RETURN void Display (void *arg)
{
    (void)arg;
    LEDrun = false;

    init();
    LCD_reset();

    while(1)
    {
        osThreadFlagsWait(0x01U, osFlagsWaitAny, osWaitForever);

        limpiardisplay();
        EscribeFraseL1(lcd_text[0]);
        EscribeFraseL2(lcd_text[1]);
    }
}

/*----------------------------------------------------------------------------
  BlinkLed: secuencia de LEDs cuando LEDrun == true
 *---------------------------------------------------------------------------*/
static __NO_RETURN void BlinkLed (void *arg)
{
    const uint8_t led_val[6] = {0x01, 0x02, 0x04, 0x08, 0x04, 0x02};
    int cnt = 0;

    LEDrun = false;
    while(1)
    {
        if (LEDrun == true)
        {
            LED_SetOut_stm(led_val[cnt]);
            if (++cnt >= sizeof(led_val)) cnt = 0;
        }
        osDelay(1000);
    }
}

/*----------------------------------------------------------------------------
  ThreadRTC: refresca el LCD con hora y fecha del RTC cada segundo.
  - Linea 1: Hora  "HH:MM:SS"
  - Linea 2: Fecha "DD-MM-YYYY"
  NO usa osThreadFlagsWait porque necesita actualizar cada segundo
  de forma autonoma, independientemente de la web.
 *---------------------------------------------------------------------------*/
static __NO_RETURN void ThreadRTC (void *arg)
{
    (void)arg;

    while(1)
    {
        /* Leer hora y fecha del RTC */
        RTC_CalendarShow(aShowTime, aShowDate);

        /* Escribir directamente en el LCD fisico */
        limpiardisplay();
        EscribeFraseL1((char *)aShowTime);
        EscribeFraseL2((char *)aShowDate);

        osDelay(1000U);
    }
}

/*----------------------------------------------------------------------------
  app_main: hilo principal
 *---------------------------------------------------------------------------*/
__NO_RETURN void app_main (void *arg)
{
    (void)arg;

    ADC_init();
    LED_Initialize_stm();

    /* Arrancamos la pila de red */
    netInitialize();

    /* IMPORTANTE: Inicializamos el hardware del RTC, pero le quitamos 
       la llamada a init_SNTP() de dentro de la función init_RTC() en tu rtc.c */
    init_RTC(); 

    /* Creamos los hilos (tu hilo del LCD ya usa colas de mensajes) */
    TID_Led     = osThreadNew(BlinkLed,  NULL, NULL);
    TID_Display = osThreadNew(Display,   NULL, NULL);
    TID_RTC     = osThreadNew(ThreadRTC, NULL, NULL);

    /* Le damos 5 segundos a la red para conseguir IP por DHCP */
    osDelay(5000U);

    /* AHORA SÍ: Forzamos la primera sincronización SNTP */
    init_SNTP();

    osThreadExit();
}