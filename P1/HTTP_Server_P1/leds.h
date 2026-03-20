

#ifndef __BOARD_LED_H
#define __BOARD_LED_H

#include <stdint.h>

extern int32_t  LED_Initialize_stm   (void); // ---> SE HA AÑADIDO A CADA FUNCION "_stm" PARA EVITAR EL ERROR DE DUPLICIDAD CON EL BOARD.
extern int32_t  LED_Uninitialize_stm (void);
extern int32_t  LED_On_stm           (uint32_t num);
extern int32_t  LED_Off_stm          (uint32_t num);
extern int32_t  LED_SetOut_stm       (uint32_t val);
extern uint32_t LED_GetCount_stm     (void);

#endif 
