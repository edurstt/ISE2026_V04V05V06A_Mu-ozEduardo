

#include "stm32f4xx_hal.h"
#include "Board_LED.h"

/* GPIO Pin identifier */
typedef struct _GPIO_PIN {
  GPIO_TypeDef *port;
  uint16_t      pin;
  uint16_t      reserved;
} GPIO_PIN;

/* LED GPIO Pins */
 const GPIO_PIN LED_PIN[] = { // PINES LED DE LA TARJETA
  { GPIOB, GPIO_PIN_0, 0},
  { GPIOB, GPIO_PIN_7, 0},
  { GPIOB, GPIO_PIN_14,0},
};

/* constante para ver cuantos leds tenemos en la tarjeta (uantas hemos metido mas bien */

#define LED_COUNT (sizeof(LED_PIN)/sizeof(GPIO_PIN)) 

/* INICIALIZACION LEDS */
int32_t LED_Initialize_stm (void) {
  GPIO_InitTypeDef GPIO_InitStruct;
  uint32_t i;

  __GPIOG_CLK_ENABLE(); //GPIO Ports Clock Enable.
 // __GPIOD_CLK_ENABLE();
 // __GPIOK_CLK_ENABLE();

  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  for (i = 0; i < LED_COUNT; i++){
    GPIO_InitStruct.Pin = LED_PIN[i].pin;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }

  return 0;
}

/* DES-INICIALIZACION LEDS */
int32_t LED_Uninitialize_stm (void) {
  uint32_t i;

  for (i = 0; i < LED_COUNT; i++) {
    HAL_GPIO_DeInit(LED_PIN[i].port, LED_PIN[i].pin);
  }

  return 0;
}



/*Funcion que realiza el encendido de un LED: Se utilizara solo en LED_SetOut_stm*/
int32_t LED_On_stm (uint32_t num) {
  int32_t retCode = 0; // control de errores 

  if (num < LED_COUNT){ // controla que se este solicitando un pin contenido en la lista
    HAL_GPIO_WritePin(LED_PIN[num].port, LED_PIN[num].pin, 1); //...se realizara el encendido.
  }
  else{
    retCode = -1;
  }

  return retCode;
}



/*Funcion que realiza el apagado de un LED: Se utilizara solo en LED_SetOut_stm*/
int32_t LED_Off_stm (uint32_t num) 
{
  int32_t retCode = 0;

  if (num < LED_COUNT){ // controla que se este solicitando un pin contenido en la lista
    HAL_GPIO_WritePin(LED_PIN[num].port, LED_PIN[num].pin, 0);
  }
  else {
    retCode = -1;
  }

  return retCode;
}

/**
  \fn          int32_t LED_SetOut (uint32_t val)
  \brief       Write value to LEDs
  \param[in]   val  value to be displayed on LEDs
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t LED_SetOut_stm (uint32_t val) {
  uint32_t n;

  for (n = 0; n < LED_COUNT; n++) {
    if (val & (1 << n)) LED_On_stm (n);
    else                LED_Off_stm(n);
  }

  return 0;
}

/**
  \fn          uint32_t LED_GetCount (void)
  \brief       Get number of LEDs
  \return      Number of available LEDs
*/
uint32_t LED_GetCount_stm (void) { // por ahora no se utiliza 

  return LED_COUNT;
}
