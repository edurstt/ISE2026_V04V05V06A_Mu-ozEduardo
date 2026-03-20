#include "main.h"
#include "Arial12x12.h"
#include <stdio.h>
#include <string.h>
#include "Driver_SPI.h"

extern ARM_DRIVER_SPI Driver_SPI1; //Sentencia para poder usar SPI.
ARM_DRIVER_SPI* SPIdrv = &Driver_SPI1;

uint8_t buffer[512]; 
uint8_t posicionL1 = 0;
uint8_t posicionL2 = 0;

TIM_HandleTypeDef htim7;


//Funcion que provoca un retardo:	
void retardo(int micro_segundos)
{	  
  //TIMER 7, 168 MHz
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 42-1;// 84 MHz / 42= 2 MHz
  htim7.Init.Period = (2-1) * micro_segundos;//2 MHz /2= 1 MHz = 1 us
  
  HAL_NVIC_EnableIRQ(TIM7_IRQn);
  __HAL_RCC_TIM7_CLK_ENABLE();
  
  HAL_TIM_Base_Init(&htim7);
  HAL_TIM_Base_Start(&htim7); 
	
	
	while(TIM7->CNT<htim7.Init.Period);
	
	HAL_TIM_Base_Stop(&htim7); 
}

//Funcion que configura el SPI:
void init(void)
{	  
  static GPIO_InitTypeDef GPIO_InitStructD;
  static GPIO_InitTypeDef GPIO_InitStructF;
  static GPIO_InitTypeDef GPIO_InitStructA;

  //Enable GPIO Clock 
  __HAL_RCC_GPIOD_CLK_ENABLE();	
  __HAL_RCC_GPIOF_CLK_ENABLE();	
  __HAL_RCC_GPIOA_CLK_ENABLE();	

  // GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_13 | GPIO_PIN_6 ;//CS, AO, RESET

  //Configure IO in ouput push-pull mode to drive external 
  GPIO_InitStructD.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructD.Pull = GPIO_NOPULL;
  GPIO_InitStructD.Speed = GPIO_SPEED_FREQ_LOW;

  //Configure IO in ouput push-pull mode to drive external 
  GPIO_InitStructF.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructF.Pull = GPIO_NOPULL;
  GPIO_InitStructF.Speed = GPIO_SPEED_FREQ_LOW;

  //Configure IO in ouput push-pull mode to drive external 
  GPIO_InitStructA.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructA.Pull = GPIO_NOPULL;
  GPIO_InitStructA.Speed = GPIO_SPEED_FREQ_LOW;
	
  //Init pines 
  GPIO_InitStructD.Pin  = GPIO_PIN_14; //CS
  HAL_GPIO_Init (GPIOD, &GPIO_InitStructD);

  GPIO_InitStructF.Pin  = GPIO_PIN_13; //AO
  HAL_GPIO_Init (GPIOF, &GPIO_InitStructF);

  GPIO_InitStructA.Pin  = GPIO_PIN_6; //RESET
  HAL_GPIO_Init (GPIOA, &GPIO_InitStructA);

  SPIdrv->Initialize(NULL);//Inicializamos SPI
  SPIdrv->PowerControl(ARM_POWER_FULL);//Modos de potencia de SPI
  SPIdrv->Control(ARM_SPI_MODE_MASTER|ARM_SPI_CPOL1_CPHA1|ARM_SPI_MSB_LSB|ARM_SPI_DATA_BITS(8),20000000);
  //Modo master, CPOL1 y CPHA1
  //8 bits de datos y frecuencia del SCLK 20 MHz

  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET); //CS
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET); //AO
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET); //RESET

  //Generamos el pulso del reset:
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);// PIN RESET a 0
  //HAL_Delay(1);//1 us
  retardo(1); //1 us
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);// Lo volvemos a encender.
  //HAL_Delay(1000);
	retardo(1000);
}

//Funcion que escribe un dato en el LCD:
void wr_data(unsigned char data)
{  
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET); //CS = 0
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET); //AO = 1  
	
  SPIdrv -> Send(&data, sizeof(data));
	//Escribimos un dato
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET); //CS = 1	
}

//Funcion que escribe un comando en el LCD:
void wr_cmd (unsigned char cmd)
{  
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET); //CS = 0
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_RESET); //AO = 0  
	
  SPIdrv->Send(&cmd, sizeof(cmd));
	//Escribimos un comando
	
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET); //CS = 1
}

//Funcion que configura el LCD:
void LCD_reset(void)
{
  wr_cmd(0xAE); //Display off
  wr_cmd(0xA2); //Fija el valor de la relación de la tensión de polarización del LCD a 1/9
  wr_cmd(0xA0); //El direccionamiento de la RAM de datos del display es la normal
  wr_cmd(0xC8); //El scan en las salidas COM es el normal
  wr_cmd(0x22); //Fija la relación de resistencias interna a 2
  wr_cmd(0x2F); //Power on
  wr_cmd(0x40); //Display empieza en la línea 0
  wr_cmd(0xAF); //Enciende el display
  wr_cmd(0x81); //Valor de Contraste
  wr_cmd(0x17); //Establecer el contraste
  wr_cmd(0xA4); //All display points normal
  wr_cmd(0xA6); //LCD Display normal
}

//Funcion que pasa la informacion al LCD:
void LCD_update(void)
{
  int i;
    
  wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
  wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
  wr_cmd(0xB0); // Pagina 0

  for(i=0;i<128;i++)
  {
    wr_data(buffer[i]);
  }

  wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
  wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
  wr_cmd(0xB1); // Pagina 1

  for(i=128;i<256;i++)
  {
    wr_data(buffer[i]);
  }

  wr_cmd(0x00);
  wr_cmd(0x10);
  wr_cmd(0xB2); //Pagina 2

  for(i=256;i<384;i++)
  {
    wr_data(buffer[i]);
  }

  wr_cmd(0x00);
  wr_cmd(0x10);
  wr_cmd(0xB3); // Pagina 3

  for(i=384;i<512;i++)
  {
    wr_data(buffer[i]);
  }

}

//Funcion que limpia el display:
void limpiardisplay(void)
{
	memset(buffer,0x00,512);
	LCD_update();
}

//Funcion que limpia la linea superior del LCD:
/*void LCD_clear_L1(void)
{
	for(int i=0; i<256; i++)
	{
		buffer[i] = 0x00;
	}
  posicionL1 = 0;
}

//Funcion que limpia la linea inferior del LCD:
void LCD_clear_L2(void)
{
	for(int i=256; i<512; i++)
	{
		buffer[i] = 0x00;
	}
  posicionL2 = 0;
}*/

//Funcion que escribe un caracter en la linea superior:
void LCD_symbolToLocalBuffer_L1(uint8_t symbol)
{
  uint8_t i, value1, value2;
  uint16_t offset = 0;
	
  offset = 25* (symbol - ' ');   
  
  for(i= 0; i<12; i++)
  {
    value1 = Arial12x12[offset+i*2+1];
    value2 = Arial12x12[offset+i*2+2];

    buffer[i+posicionL1] = value1; //Valores para la Pagina 0.
    buffer[i+128+posicionL1] = value2; //Valores para la Pagina 1.
  }
  posicionL1 = posicionL1 + Arial12x12[offset];
}

//Funcion que escribe un caracter en la linea inferior:
void LCD_symbolToLocalBuffer_L2(uint8_t symbol)
{
  uint8_t i, value1, value2;
  uint16_t offset= 0;
	
  offset = 25* (symbol - ' '); 
	
  for(i= 0; i<12; i++)  
  {
    value1 = Arial12x12[offset+i*2+1];
    value2 = Arial12x12[offset+i*2+2];

    buffer[i+256+posicionL2] = value1; 
    buffer[i+384+posicionL2] = value2;
  }
  posicionL2 = posicionL2 + Arial12x12[offset];
}

//Funcion que escribe una frase en la linea superior:
/*void escribeLinea1(char cadena[])
{
  int i;
  
	for(i=0; i<strlen(cadena); i++)
  {
    LCD_symbolToLocalBuffer_L1(cadena[i]);
  }

  LCD_update();
}

//Funcion que escribe una frase en la linea inferior:
void escribeLinea2(char cadena[])
{
  int i;
  
	for(i=0; i<strlen(cadena); i++)
  {
    LCD_symbolToLocalBuffer_L2(cadena[i]);
  }

  LCD_update();
}*/

//Funcion que escribe una frase en la linea superior:
void EscribeFraseL1(const char *frase1)
{
	int longitud = strlen(frase1);
	int z;
  
	posicionL1 = 0; //OJO: RESETEAMOS posicionL1 PARA EVITAR QUE EL TEXTO SE VAYA DESPLAZANDO POR TODA LA LINEA CADA VEZ QUE PULSEMOS Send!!!
	
	for(z=0; z<longitud; z++)
  {	
    if(posicionL1<128-12) //OJO: REALIZAMOS ESTO PARA EVITAR EL DESBORDAMIENTO DEL BUFFER!!!
		{			
			LCD_symbolToLocalBuffer_L1(frase1[z]);
			LCD_update();
		}
	}
}

//Funcion que escribe una frase en la linea inferior:
void EscribeFraseL2(const char *frase2)
{
	int longitud = strlen(frase2);
  int z;
	
	posicionL2 = 0; //OJO: RESETEAMOS posicionL2 PARA EVITAR QUE EL TEXTO SE VAYA DESPLAZANDO POR TODA LA LINEA CADA VEZ QUE PULSEMOS Send!!!
	
	for(z=0; z<longitud; z++)
  {
    if(posicionL2<128-12)	//OJO: REALIZAMOS ESTO PARA EVITAR EL DESBORDAMIENTO DEL BUFFER!!!	
		{		
			LCD_symbolToLocalBuffer_L2(frase2[z]);
			LCD_update();
		}
	}
}

void symbolToLocalBuffer(uint8_t line,uint8_t symbol)
{  
  if(line==1)
   LCD_symbolToLocalBuffer_L1(symbol);
  if(line==2)
    LCD_symbolToLocalBuffer_L2(symbol);
}
