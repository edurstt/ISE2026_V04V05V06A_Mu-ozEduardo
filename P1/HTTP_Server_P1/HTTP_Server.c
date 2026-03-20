
#include <stdio.h>
#include "main.h"
#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE
#include "stm32f4xx_hal.h"              // Keil::Device:STM32Cube HAL:Common
#include "leds.h"
#include "lcd.h"
#include "adc.h"

// Main stack size must be multiple of 8 Bytes
#define APP_MAIN_STK_SZ (1024U)
uint64_t app_main_stk[APP_MAIN_STK_SZ / 8];
const osThreadAttr_t app_main_attr = {
  .stack_mem  = &app_main_stk[0],
  .stack_size = sizeof(app_main_stk)
};

//extern uint16_t AD_in          (uint32_t ch);
//extern uint8_t  get_button     (void);
//extern void     netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len);

//Hilos:
extern osThreadId_t TID_Display;
extern osThreadId_t TID_Led;
osThreadId_t TID_Display;
osThreadId_t TID_Led;

//Variables:
bool LEDrun;
char lcd_text[2][20+1];
int i;
ADC_HandleTypeDef adchandle; 
float value = 0;

//Funciones:
__NO_RETURN void app_main (void *arg);
static void BlinkLed (void *arg);
static void Display  (void *arg);



/*----------------------------------------------------------------------------
																ADC handler
 *---------------------------------------------------------------------------*/
/*************************************************************************************************************************
* EXPLICACION: mediante las funciones de configuracion de pines, arranque de la conversion y obtencion de resultados,    *
* creamos la funcion AD_in en este archivo que corresponde al servidor web y simplemente invocamos ADC_StartConversion   *
* para el arranque del ADC y obtenemos el resultado de la conversion con ADC_getVoltage.															   *
**************************************************************************************************************************/

//Funcion que realiza la lectura del valor del Potenciometro 1 (ADC):
uint16_t AD_in (uint32_t ch) {
  int32_t val = 0;

  if (ch == 0) {
    val = ADC_getValue(ADC_CHANNEL_10);
  }
  return ((uint16_t)val);
}
/*----------------------------------------------------------------------------
																FIN ADC handler
 *---------------------------------------------------------------------------*/

/* Read digital inputs */
uint8_t get_button (void) {
  //return ((uint8_t)Buttons_GetState ());
}

/* IP address change notification */
void netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len) {

  /*(void)if_num;
  (void)val;
  (void)len;

  if (option == NET_DHCP_OPTION_IP_ADDRESS) {
    //IP address change, trigger LCD update 
    osThreadFlagsSet (TID_Display, 0x01);
  }*/
}

/*----------------------------------------------------------------------------
														LCD display handler
 *---------------------------------------------------------------------------*/
/***************************************************************************************************
* EXPLICACION: en esta fucnion se realiza la representacion de los textos escritos en la web. Es   *
* importante destacar que la inicializacion y el reset del LCD se realiza en esta funcion y no     *
* en el main del servidor o en el main del microprocesador (si se hace esto, se bloquea). Se       *
* utiliza la señal 0x01U proporcionada por el proyecto base, que cada vez que se introduce un      *
* texto desde la web, se envia desde el cgi hasta esta funcion, que actualizara el LCD.            *
****************************************************************************************************/

//Funcion que realiza las representaciones en el LCD:
static __NO_RETURN void Display (void *arg) 
{
	(void)arg;
	LEDrun = false;
	
	static char buf[24];

	init(); //IMPORTANTE --> inicializar y resetear el LCD aqui y no en el main del micro o en el del servidor.
	LCD_reset();

	EscribeFraseL1(lcd_text[0]);
	EscribeFraseL2(lcd_text[1]);

  while(1) 
	{
		osThreadFlagsWait (0x01U, osFlagsWaitAny, osWaitForever); //Se utiliza el hilo que se proporciona por defecto. La señal proviene del archivo HTTP_Server_CGI.
		
		sprintf (buf, "%-20s", lcd_text[0]);
		sprintf (buf, "%-20s", lcd_text[1]);
		
		limpiardisplay();
		EscribeFraseL1(lcd_text[0]);		
		EscribeFraseL2(lcd_text[1]);
  }
}
/*----------------------------------------------------------------------------
														FIN LCD display handler
 *---------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
																LEDs handler
 *---------------------------------------------------------------------------*/
/**********************************************************************************************
 * EXPLICACION: en esta funcion se realiza el barrido de los LED, primeramente se han         *
 * modificado los valores de led_val para el correcto encendido-apagado y el resto de la      *
 * funcion se ha mantenido como en el ejemplo de partida. Cabe destacar la funcion            *
 * LED_SetOut, que es la encargada de realizar el ON y OFF de los LED. Recibe como            *
 * parametro el contador para saber que LED debe encenderse.																	*
 **********************************************************************************************/

//Funcion que realiza el barrido de los LEDs:
static __NO_RETURN void BlinkLed (void *arg) 
{
  const uint8_t led_val[6] = {0x01,0x02,0x04,0x08,0x04,0x02};
  int cnt = 0;

  LEDrun = false; //Para que al iniciar el servidor no empiece con el barrido.
  while(1) 
  {    
    if (LEDrun == true) //Cada 100 ms. // si esta en modo Running lights
    {
      LED_SetOut_stm (led_val[cnt]);
			
      if (++cnt >= sizeof(led_val)) 
      {
        cnt = 0;
      }
    }
    osDelay (1000);
  }
}

/*----------------------------------------------------------------------------
  Main Thread 'main': Run Network
 *---------------------------------------------------------------------------*/
__NO_RETURN void app_main (void *arg) 
{
	(void)arg;
  ADC_init();
	netInitialize();      // inicializa la red
	LED_Initialize_stm(); // inicializa fisicamente los leds
	
  TID_Led     = osThreadNew (BlinkLed, NULL, NULL);
  TID_Display = osThreadNew (Display,  NULL, NULL);

  osThreadExit();
}
