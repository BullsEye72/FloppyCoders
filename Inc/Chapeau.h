
// screen max size
#define MAX_X 800
#define MAX_Y 480
#define MIN_X 0
#define MIN_Y 0

/** LEDs Settings *********************************/
#define LED_NUMBER  20
#define LED_ALL     0xFF
/***********************************************/


void service_Chapeau_task(void  const * argument);
void service_ChapeauLed_task(void  const * argument);
void service_ChapeauUart_task(void  const * argument);



