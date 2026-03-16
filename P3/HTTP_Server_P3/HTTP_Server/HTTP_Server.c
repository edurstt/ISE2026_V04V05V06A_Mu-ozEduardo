/*----------------------------------------------------------------------------
* INGENIERIA DE SISTEMAS ELECTRONICOS - ETSIST (UPM)
* PRACTICA 3: Modos de bajo consumo (Sleep Mode)
*---------------------------------------------------------------------------*/

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

/* Hilos de P2 */
extern osThreadId_t TID_Display;
extern osThreadId_t TID_Led;
volatile bool flag_despertar = false; 

osThreadId_t TID_Display;
osThreadId_t TID_Led;
osThreadId_t TID_RTC;

/* Hilos nuevos de P3 */
osThreadId_t TID_BlinkVerde;
osThreadId_t TID_PowerCtrl;

bool LEDrun;
char lcd_text[2][20+1];
int i;
ADC_HandleTypeDef adchandle;
float value = 0;

uint8_t aShowTime[50] = {0};
uint8_t aShowDate[50] = {0};

/* Prototipos */
__NO_RETURN void app_main (void *arg);
static void BlinkLed  (void *arg);
static void Display   (void *arg);
static void ThreadRTC (void *arg);
static void ThreadBlinkVerde (void *arg);
static void ThreadPowerCtrl  (void *arg);

/* --- Funciones de soporte --- */
uint16_t AD_in (uint32_t ch) {
  int32_t val = 0;
  if (ch == 0) { val = ADC_getValue(ADC_CHANNEL_10); }
  return ((uint16_t)val);
}
uint8_t get_button (void) { return 0; }
void netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len) {}

/*----------------------------------------------------------------------------
  NUEVO P3: ThreadBlinkVerde -> Indicador de modo RUN
  Si la CPU entra en Sleep, este parpadeo (100ms) se detiene.
*---------------------------------------------------------------------------*/
static __NO_RETURN void ThreadBlinkVerde (void *arg) {
  while(1) {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); // LED Verde
    osDelay(100U);
  }
}

/*----------------------------------------------------------------------------
  NUEVO P3: ThreadPowerCtrl -> Gestión del modo Sleep
*---------------------------------------------------------------------------*/
static __NO_RETURN void ThreadPowerCtrl (void *arg) {
    osDelay(5000U); 

    while(1) {
        // 1. Ejecución normal durante 15 segundos
        osDelay(15000U); 

        // 2. Preparar el modo Sleep
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // LED Rojo ON

        // --- SECCIÓN CRÍTICA ---
        flag_despertar = false; 

        // IMPORTANTE: Detener el SysTick para que el RTOS no despierte al micro cada 1ms
        HAL_SuspendTick(); 

        while (flag_despertar == false) {
            // Entrar en modo Sleep. El micro se detendrá aquí.
            // Solo una interrupción (como la del botón) lo sacará de esta línea.
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

            // Si despertó por algo que NO es el botón (ej. un sensor), 
            // el bucle while lo volverá a dormir inmediatamente.
        }

        // 3. Al despertar por el botón
        HAL_ResumeTick(); 
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // LED Rojo OFF

        osDelay(1000U); // Debounce y vuelta a empezar
    }
}

/*----------------------------------------------------------------------------
  Display: gestiona texto enviado desde la web al LCD
*---------------------------------------------------------------------------*/
static __NO_RETURN void Display (void *arg) {
    (void)arg;
    LEDrun = false;
    init();
    LCD_reset();
    while(1) {
        osThreadFlagsWait(0x01U, osFlagsWaitAny, osWaitForever);
        //limpiardisplay();
        EscribeFraseL1(lcd_text[0]);
        EscribeFraseL2(lcd_text[1]);
    }
}

/*----------------------------------------------------------------------------
  BlinkLed: secuencia de LEDs web
*---------------------------------------------------------------------------*/
static __NO_RETURN void BlinkLed (void *arg) {
    const uint8_t led_val[6] = {0x01, 0x02, 0x04, 0x08, 0x04, 0x02};
    int cnt = 0;
    LEDrun = false;
    while(1) {
        if (LEDrun == true) {
            LED_SetOut_stm(led_val[cnt]);
            if (++cnt >= sizeof(led_val)) cnt = 0;
        }
        osDelay(1000);
    }
}

/*----------------------------------------------------------------------------
  ThreadRTC: refresca el LCD con hora y fecha
*---------------------------------------------------------------------------*/
static __NO_RETURN void ThreadRTC (void *arg) {
    (void)arg;
    while(1) {
        RTC_CalendarShow(aShowTime, aShowDate);

        // NOTA: Si el LCD parpadea mucho cada segundo (efecto "reseteo visual"),
        // es por culpa de esta llamada a limpiardisplay(). Es normal.
        //limpiardisplay(); 
        EscribeFraseL1((char *)aShowTime);
        EscribeFraseL2((char *)aShowDate);
        osDelay(1000U);
    }
}

/*----------------------------------------------------------------------------
  app_main: hilo principal
*---------------------------------------------------------------------------*/
__NO_RETURN void app_main (void *arg) {
    (void)arg;

    ADC_init();
    LED_Initialize_stm();

    /* Arrancamos la pila de red */
    netInitialize();

    /* Inicializamos el hardware del RTC (que además configura el botón azul) */
    init_RTC(); 

    /* Creamos los hilos de la P2 */
    TID_Led     = osThreadNew(BlinkLed,  NULL, NULL);
    TID_Display = osThreadNew(Display,   NULL, NULL);
    TID_RTC     = osThreadNew(ThreadRTC, NULL, NULL);

    /* Creamos los hilos de la P3 (Bajo Consumo) */
    TID_BlinkVerde = osThreadNew(ThreadBlinkVerde, NULL, NULL);
    TID_PowerCtrl  = osThreadNew(ThreadPowerCtrl,  NULL, NULL);

    /* Le damos 5 segundos a la red para conseguir IP por DHCP */
    osDelay(5000U);

    /* Forzamos la primera sincronización SNTP */
    init_SNTP();

    osThreadExit();
}