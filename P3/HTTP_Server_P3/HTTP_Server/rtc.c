#include <stdio.h>
#include <string.h>
#include <time.h>
#include "rtc.h"
#include "lcd.h"
#include "rl_net.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

/* Variable global del main.c para silenciar su ruleta de LEDs */
extern bool LEDrun;
extern volatile bool flag_despertar;

/* IP de Google directa para saltarnos el DNS del laboratorio */
const NET_ADDR4 ntp_server = { NET_ADDR_IP4, 123, 216, 239, 35, 4 };

RTC_HandleTypeDef RtcHandle;

/* RTOS Resources */
static osThreadId_t tid_ThAlarm;
static osTimerId_t  tim_id_verde;
static osTimerId_t  tim_id_rojo;
static osTimerId_t  tim_id_3min;

/* Banderas para no bloquear el procesador */
#define FLAG_ALARM  0x01U
#define FLAG_BOTON  0x02U
#define FLAG_SYNC   0x04U

static volatile int cnt_verde = 0;
static volatile int cnt_rojo  = 0;

/* Prototipos internos */
static void time_callback(uint32_t seconds, uint32_t seconds_fraction);
void init_SNTP(void);

/* ------------------ CONFIGURACIÓN DEL HARDWARE ------------------ */

void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    RCC_OscInitTypeDef        RCC_OscInitStruct   = {0};
    RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.LSEState       = RCC_LSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection    = RCC_RTCCLKSOURCE_LSE;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    __HAL_RCC_RTC_ENABLE();
}

void RTC_CalendarConfig(struct tm ts)
{
    RTC_DateTypeDef sdatestructure = {0};
    RTC_TimeTypeDef stimestructure = {0};

    sdatestructure.Year    = ts.tm_year - 100;
    sdatestructure.Month   = ts.tm_mon + 1;
    sdatestructure.Date    = ts.tm_mday;
    sdatestructure.WeekDay = RTC_WEEKDAY_MONDAY;

    HAL_RTC_SetDate(&RtcHandle, &sdatestructure, RTC_FORMAT_BIN);

    stimestructure.Hours          = ts.tm_hour;
    stimestructure.Minutes        = ts.tm_min;
    stimestructure.Seconds        = ts.tm_sec;
    stimestructure.TimeFormat     = 0x00; 
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

    HAL_RTC_SetTime(&RtcHandle, &stimestructure, RTC_FORMAT_BIN);

    HAL_RTCEx_BKUPWrite(&RtcHandle, RTC_BKP_DR1, 0x32F2);
}

void RTC_CalendarShow(uint8_t *showtime, uint8_t *showdate)
{
    RTC_DateTypeDef sdatestructureget;
    RTC_TimeTypeDef stimestructureget;

    HAL_RTC_GetTime(&RtcHandle, &stimestructureget, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&RtcHandle, &sdatestructureget, RTC_FORMAT_BIN);

    sprintf((char *)showtime, "%02d:%02d:%02d", 
            stimestructureget.Hours, stimestructureget.Minutes, stimestructureget.Seconds);

    sprintf((char *)showdate, "%02d-%02d-20%02d", 
            sdatestructureget.Date, sdatestructureget.Month, sdatestructureget.Year);
}

void RTC_AlarmConfig(void)
{
    RTC_AlarmTypeDef sAlarm = {0};
    HAL_RTC_DeactivateAlarm(&RtcHandle, RTC_ALARM_A);

    sAlarm.AlarmTime.Hours          = 0;
    sAlarm.AlarmTime.Minutes        = 0;
    sAlarm.AlarmTime.Seconds        = 0; 
    sAlarm.AlarmTime.TimeFormat     = 0x00; 

    sAlarm.AlarmMask = RTC_ALARMMASK_HOURS | RTC_ALARMMASK_MINUTES | RTC_ALARMMASK_DATEWEEKDAY;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    sAlarm.Alarm              = RTC_ALARM_A;

    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

    HAL_RTC_SetAlarm_IT(&RtcHandle, &sAlarm, RTC_FORMAT_BIN);
}

/* ------------------ ISRs (Solo levantan banderas) ------------------ */

void RTC_Alarm_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&RtcHandle);
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    osThreadFlagsSet(tid_ThAlarm, FLAG_ALARM);
}

void Pulsador_config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Pin  = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING; 
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == GPIO_PIN_13) {
        // Tu código de la alarma original
        osThreadFlagsSet(tid_ThAlarm, FLAG_BOTON);

        // ¡LA CLAVE! Le damos permiso al micro para despertar
        flag_despertar = true; 
    }

}

/* ------------------ TIMERS Y CALLBACKS (LEDS) ------------------ */

static void Timer_Callback_Verde(void *arg)
{
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); 
    if (++cnt_verde >= 10) { /* 10 toggles a 500ms = 5 segundos */
        cnt_verde = 0;
        osTimerStop(tim_id_verde);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    }
}

static void Timer_Callback_Rojo(void *arg)
{
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14); 
    if (++cnt_rojo >= 40) { /* 40 toggles a 100ms = 4 segundos a 5Hz */
        cnt_rojo = 0;
        osTimerStop(tim_id_rojo);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    }
}

static void Timer_Callback_3min(void *arg)
{
    osThreadFlagsSet(tid_ThAlarm, FLAG_SYNC);
}



/* ------------------ CEREBRO DEL RTOS ------------------ */

static void ThAlarm(void *argument)
{
    uint32_t flags;
    struct tm ts_2000 = {0};

    ts_2000.tm_year = 100; // 2000
    ts_2000.tm_mon  = 0;   // Enero
    ts_2000.tm_mday = 1;   // Día 1

    while (1)
    {
        flags = osThreadFlagsWait(FLAG_ALARM | FLAG_BOTON | FLAG_SYNC, osFlagsWaitAny, osWaitForever);

        /* Salta la alarma a los :00 segundos -> LED VERDE (5 segundos) */
        if (flags & FLAG_ALARM) {
            LEDrun = false; /* Silenciamos el main.c */
            cnt_verde = 0;
            osTimerStart(tim_id_verde, 500U); 
        }

        /* Pulsan botón AZUL -> Resetea a 2000 y reinicia timers */
        if (flags & FLAG_BOTON) {
           // RTC_CalendarConfig(ts_2000); 
            osTimerStop(tim_id_3min);
            osTimerStart(tim_id_3min, 180000U); 
        }

        /* Piden sincronizar -> Llama a SNTP */
        if (flags & FLAG_SYNC) {
            init_SNTP();
        }
    }
}

/* ------------------ SNTP ------------------ */

static void time_callback(uint32_t seconds, uint32_t seconds_fraction)
{
    struct tm *ptr_ts;

    if (seconds == 0U) {
        osTimerStart(tim_id_3min, 5000U);
        return;
    }

    time_t sys_time = (time_t)seconds + 3600; 
    ptr_ts = localtime(&sys_time);

    if (ptr_ts != NULL) {
        RTC_CalendarConfig(*ptr_ts);
    }

    /* ¡Llegó la hora de internet! -> LED ROJO (4 segundos a 5Hz) */
    LEDrun = false; /* Silenciamos el main.c */
    cnt_rojo = 0;
    osTimerStart(tim_id_rojo, 100U);
    osTimerStart(tim_id_3min, 180000U);
}

void init_SNTP(void)
{
    if (netSNTPc_GetTime((NET_ADDR *)&ntp_server, time_callback) != netOK) {
        osTimerStart(tim_id_3min, 2000U);
    }
}

/* ------------------ INICIALIZACIÓN ------------------ */

void init_RTC(void)
{
    /* Activamos reloj del puerto B a la fuerza por si acaso */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_14; /* Verde y Rojo */
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    RtcHandle.Instance          = RTC;
    RtcHandle.Init.HourFormat   = RTC_HOURFORMAT_24;
    RtcHandle.Init.AsynchPrediv = 127;
    RtcHandle.Init.SynchPrediv  = 255;

    __HAL_RTC_RESET_HANDLE_STATE(&RtcHandle);
    HAL_RTC_Init(&RtcHandle);

    if (HAL_RTCEx_BKUPRead(&RtcHandle, RTC_BKP_DR1) != 0x32F2)
    {
        struct tm ts_2000 = {0};
        ts_2000.tm_year = 100;
        ts_2000.tm_mon  = 0;
        ts_2000.tm_mday = 1;
        RTC_CalendarConfig(ts_2000); 
    }

    tim_id_verde = osTimerNew(Timer_Callback_Verde, osTimerPeriodic, NULL, NULL);
    tim_id_rojo  = osTimerNew(Timer_Callback_Rojo,  osTimerPeriodic, NULL, NULL);
    tim_id_3min  = osTimerNew(Timer_Callback_3min,  osTimerPeriodic, NULL, NULL);

    tid_ThAlarm = osThreadNew(ThAlarm, NULL, NULL);
    Pulsador_config();
    RTC_AlarmConfig();
}