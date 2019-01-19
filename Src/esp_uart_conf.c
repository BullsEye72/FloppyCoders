
#include "platform_init.h"

UART_HandleTypeDef *gEsp8266UART;
extern UART_HandleTypeDef  esp8266UART;

#define USARTx                           UART5
#define USARTx_CLK_ENABLE()              __UART5_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __GPIOD_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __GPIOC_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __UART5_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __UART5_RELEASE_RESET()

/* Definition for USARTx Pins */
#define USARTx_TX_PIN                    GPIO_PIN_12
#define USARTx_TX_GPIO_PORT              GPIOC
#define USARTx_TX_AF                     GPIO_AF8_UART5
#define USARTx_RX_PIN                    GPIO_PIN_2
#define USARTx_RX_GPIO_PORT              GPIOD
#define USARTx_RX_AF                     GPIO_AF8_UART5

/* Definition for USARTx's NVIC IRQ and IRQ Handlers */
#define USARTx_IRQn                      UART5_IRQn
#define USARTx_IRQHandler                UART5_IRQHandler

/* WiFi module Reset pin definitions */
#define ESP8266_RST_GPIO_PORT            GPIOJ
#define ESP8266_RST_PIN                  GPIO_PIN_14
#define ESP8266_RST_GPIO_CLK_ENABLE()    __GPIOJ_CLK_ENABLE()

#define ESP8266_GPIO0_GPIO_PORT         GPIOJ
#define ESP8266_GPIO0_PIN               GPIO_PIN_4
#define ESP8266_GPIO0_GPIO_CLK_ENABLE() __GPIOJ_CLK_ENABLE()
uint32_t esp_hw_term(void);

uint32_t esp_hw_reset(uint32_t state )
{
/* Set the RST IO high */
HAL_GPIO_WritePin(ESP8266_RST_GPIO_PORT, ESP8266_RST_PIN,state ? GPIO_PIN_SET :  GPIO_PIN_RESET);
  return 0;
}



uint32_t esp_hw_enable_GPIO0(uint32_t state)
{

  if(state)
  {
    HAL_GPIO_WritePin(ESP8266_GPIO0_GPIO_PORT, ESP8266_GPIO0_PIN,GPIO_PIN_SET );
  }
  else
  {
    HAL_GPIO_WritePin(ESP8266_GPIO0_GPIO_PORT, ESP8266_GPIO0_PIN,GPIO_PIN_RESET);
  }
  return 0;
}





uint32_t esp_hw_init(UART_HandleTypeDef *pHandle)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  ESP8266_GPIO0_GPIO_CLK_ENABLE();
  /* Set the GPIO pin configuration parametres */
  GPIO_InitStruct.Pin       = ESP8266_GPIO0_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(ESP8266_GPIO0_GPIO_PORT, &GPIO_InitStruct);



  /* The ESP8266 'RST' IO must remain high during communication with WiFi module*/
  
  /* Enable the GPIO clock */
  ESP8266_RST_GPIO_CLK_ENABLE();
  
  /* Set the GPIO pin configuration parametres */
  GPIO_InitStruct.Pin       = ESP8266_RST_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;

  /* Configure the RST IO */
  HAL_GPIO_Init(ESP8266_RST_GPIO_PORT, &GPIO_InitStruct);
  
  
  /* Wait for the device to be ready */
  HAL_Delay(500);

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  USARTx_TX_GPIO_CLK_ENABLE();
  USARTx_RX_GPIO_CLK_ENABLE();

  /* Enable USARTx clock */
  USARTx_CLK_ENABLE(); 
  
  /*##-2- Configure peripheral GPIO ##########################################*/  
  /* UART TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = USARTx_TX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = USARTx_TX_AF;

  HAL_GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStruct);

  /* UART RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = USARTx_RX_PIN;
  GPIO_InitStruct.Alternate = USARTx_RX_AF;

  HAL_GPIO_Init(USARTx_RX_GPIO_PORT, &GPIO_InitStruct);
    
  /*##-3- Configure the NVIC for UART ########################################*/
  /* NVIC for USART */
  HAL_NVIC_SetPriority(USARTx_IRQn, 0, 1);
  /* By default, USARTx_IRQn   as an higher priority than the kernel, we need to reduce it, in order to allow rtos syscall in the ISR */
  NVIC_SetPriority(USARTx_IRQn, (configMAX_SYSCALL_INTERRUPT_PRIORITY >> 4));
  HAL_NVIC_EnableIRQ(USARTx_IRQn);
  /* Reset the esp and change its boot mode as FW download*/
  
  /* Enable the reset  */
  esp_hw_reset(0);
  HAL_Delay(100);
  /* Set the FW download mode */
  esp_hw_enable_GPIO0(1);
  esp_hw_reset(1);
  
  return 1;
}
uint32_t esp_hw_term()
{
  return 1;
}


