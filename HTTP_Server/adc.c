#include "adc.h"
#include "stm32f4xx_hal.h"

#define RESOLUTION_12B 4096U
#define VREF 3.3f

static ADC_HandleTypeDef hadc1;

/**
  * @brief Initialize the ADC to work with single conversions. 12 bits resolution, software start, 1 conversion
  * @param none
  * @retval HAL_StatusTypeDef HAL_ADC_Init
  */
int  ADC_init(){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
  /*PC0     ------> ADC1_IN10
    PC3     ------> ADC1_IN13
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
      return -1;
    else
      return 0;
  }
/**
  * @brief Configure a specific channels ang gets the voltage in float type. This funtion calls to  HAL_ADC_PollForConversion that needs HAL_GetTick()
  * @param ADC_HandleTypeDef
	* @param channel number
	* @retval voltage in float (resolution 12 bits and VRFE 3.3
  */
int32_t ADC_getValue(uint32_t Channel){
    ADC_ChannelConfTypeDef sConfig = {0};
    HAL_StatusTypeDef status;
    
    int32_t raw = 0;
    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
    sConfig.Channel = Channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK){
      return -1;
    }
    
    HAL_ADC_Start(&hadc1);
    do
      status = HAL_ADC_PollForConversion(&hadc1, 0); //This funtions uses the HAL_GetTick(), then it only can be executed wehn the OS is running
    while(status != HAL_OK);
    raw = (int32_t)HAL_ADC_GetValue(&hadc1);
    return raw;
  }
	
/* Example of using this code from a Thread 
void Thread (void *argument) {
  ADC_HandleTypeDef adchandle; //handler definition
	ADC1_pins_F429ZI_config(); //specific PINS configuration
	float value;
	ADC_Init_Single_Conversion(&adchandle , ADC1); //ADC1 configuration
  while (1) {
    
	  value=ADC_getVoltage(&adchandle , 10 ); //get values from channel 10->ADC123_IN10
		value=ADC_getVoltage(&adchandle , 13 );
		osDelay(1000);
   
  }
}
*/
