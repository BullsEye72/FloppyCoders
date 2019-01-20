
// screen max size
#define MAX_X 800
#define MAX_Y 480
#define MIN_X 0
#define MIN_Y 0

/** LEDs Settings *********************************/
#define LED_NUMBER  20
#define LED_ALL     0xFF

/* Definition for SPIx clock resources */
#define SPIx                             SPI2
#define SPI2_CLK_ENABLE()                __HAL_RCC_SPI2_CLK_ENABLE()
#define SPI2_SCK_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI2_MISO_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define SPI2_MOSI_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()

#define SPI2_FORCE_RESET()               __HAL_RCC_SPI2_FORCE_RESET()
#define SPI2_RELEASE_RESET()             __HAL_RCC_SPI2_RELEASE_RESET()

/* Definition for SPIx Pins */
#define SPI2_SCK_PIN                     GPIO_PIN_12
#define SPI2_SCK_GPIO_PORT               GPIOA
#define SPI2_SCK_AF                      GPIO_AF5_SPI2
#define SPI2_MISO_PIN                    GPIO_PIN_14
#define SPI2_MISO_GPIO_PORT              GPIOB
#define SPI2_MISO_AF                     GPIO_AF5_SPI2
#define SPI2_MOSI_PIN                    GPIO_PIN_15
#define SPI2_MOSI_GPIO_PORT              GPIOB
#define SPI2_MOSI_AF                     GPIO_AF5_SPI2

/***********************************************/

/** UART Settings *********************************************/

UART_HandleTypeDef UartHandleChapeau;
#define TXBUFFERSIZE  30
#define RXBUFFERSIZE  TXBUFFERSIZE /* Both Rx & Tx buffers have the same fixed size */
#define MSG_END_CHAR  0x0A /* LF */
#define MSG_STUF_CHAR ' '  /* space */
uint8_t aTxBuffer[TXBUFFERSIZE];
uint8_t aRxBuffer[RXBUFFERSIZE];

#define USARTChapeau                           USART6
#define USARTChapeau_CLK_ENABLE()              __USART6_CLK_ENABLE()
#define USARTChapeau_RX_GPIO_CLK_ENABLE()      __GPIOC_CLK_ENABLE()
#define USARTChapeau_TX_GPIO_CLK_ENABLE()      __GPIOC_CLK_ENABLE()

#define USARTChapeau_FORCE_RESET()             __USART6_FORCE_RESET()
#define USARTChapeau_RELEASE_RESET()           __USART6_RELEASE_RESET()

/* Definition for USARTx Pins */
#define USARTChapeau_TX_PIN                    GPIO_PIN_6
#define USARTChapeau_TX_GPIO_PORT              GPIOC
#define USARTChapeau_TX_AF                     GPIO_AF8_USART6
#define USARTChapeau_RX_PIN                    GPIO_PIN_7
#define USARTChapeau_RX_GPIO_PORT              GPIOC
#define USARTChapeau_RX_AF                     GPIO_AF8_USART6

/* Definition for USARTx's NVIC */
#define USARTChapeau_IRQn                      USART6_IRQn
#define USARTChapeau_IRQHandler                USART6_IRQHandler

/***********************************************/



void service_Chapeau_task(void  const * argument);
void service_ChapeauLed_task(void  const * argument);
void service_ChapeauUart_task(void  const * argument);

static void Error_Handler(void);
static uint16_t Buffercmp(uint8_t *pBuffer1, uint8_t *pBuffer2, uint16_t BufferLength);
