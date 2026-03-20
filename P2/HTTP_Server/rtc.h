#ifndef __RTC_H
#define __RTC_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <time.h> /* Añadimos esta libreria para que entienda struct tm */

/* Handles globales accesibles desde HTTP_Server_CGI.c */
extern RTC_HandleTypeDef RtcHandle;
extern RTC_DateTypeDef   sdatestructure;
extern RTC_TimeTypeDef   stimestructure;

/* API publica */
void init_RTC(void);
void init_SNTP(void);
void RTC_CalendarConfig(struct tm ts); /* ¡AQUÍ ESTÁ EL CAMBIO! */
void RTC_AlarmConfig(void);
void RTC_CalendarShow(uint8_t *showtime, uint8_t *showdate);

#endif