/**
 ******************************************************************************
 * @file    Chapeau.c
 * @author  MCD Application Team
 * @version V1.0.0
 * @date    04-December-2018
 * @brief   Ademo file for 24h code 2019 contest
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2018 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#include "service.h"
#include "platform_init.h"
#include "cmsis_os.h"
#include "Chapeau.h"
#include "stdlib.h"
#include "stm32f7xx_hal_msp.h"

#define NB_RETRY_RELEASE_DETECTION 10 /* 10 * 5ms = 50ms */

TS_StateTypeDef  TS_State = {0};

uint16_t maxX, minX, maxY, minY;

typedef struct point_t
{
	uint16_t x;
	uint16_t y;
}point;

#define MAX_POINTS 200
#define MAX_SERIES 20
typedef struct series_t
{
	point points [MAX_POINTS];
	uint16_t points_nb;
}series;

typedef struct image_t
{
	series point_series [MAX_SERIES];
	uint16_t series_nb;
}image;

void resetTouchInfos(){
	maxX=0;
	maxY=0;
	minY=480;
	minX=800;
}

void drawline(int x0, int y0, int x1, int y1, int width, int height, char **imageWithLine)
{
	point p0; point p1;

	int32_t dx, sx, dy, sy, err, e2;
	p0.x=x0;
	p0.y=y0;
	p1.x=x1;
	p1.y=y1;
	dx	= abs(p1.x - p0.x);
	sx	= p0.x < p1.x ? 1 : -1;
	dy	= abs(p1.y - p0.y);
	sy	= p0.y < p1.y ? 1 : -1;
	err	= (dx > dy ? dx : -dy) / 2;

	for (;;) {
		/* check that we don't overflow*/
		if( (p0.x < width) && (p0.y < height)) {
			imageWithLine[p0.x][p0.y]=0;

			/* For debug only */
			/*BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
			BSP_LCD_FillCircle(p0.x+10,p0.y+10,1);*/
		}

		if (p0.x == p1.x && p0.y == p1.y)
			break;
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			p0.x += sx;
		}
		if (e2 <  dy) {
			err += dx;
			p0.y += sy;
		}
	}
}


void create_bitmap(image *imageNew, char **bitmap28x28)
{
	float facteur = 20;
	int Width, Height;
	int Width_Rescaled=28, Height_Rescaled=28;

	Width = maxX - minX + 2;
	Height = maxY - minY + 2;

	float factx = (Width)/facteur;
	float facty = (Height) /facteur;

	/* Update point series for 20x20 Image */
	for (int i = 0; i < imageNew->series_nb; i++) {
		for (int j = 0; j < imageNew->point_series[i].points_nb; j++) {
			imageNew->point_series[i].points[j].x = (int) floor((imageNew->point_series[i].points[j].x-minX)/factx);
			imageNew->point_series[i].points[j].y = (int) floor((imageNew->point_series[i].points[j].y-minY)/facty);
		}
	}

	for (int w =0; w < Width_Rescaled; w++)
		for (int h= 0 ; h < Height_Rescaled ; h++)  bitmap28x28[w][h]  = 0xff;

	for (int i = 0; i < imageNew->series_nb; i++) {
		for (int j = 0; j < imageNew->point_series[i].points_nb - 1; j++) {
			drawline(imageNew->point_series[i].points[j].x+4,
					imageNew->point_series[i].points[j].y+4,
					imageNew->point_series[i].points[j+1].x+4,
					imageNew->point_series[i].points[j+1].y+4,
					Width_Rescaled,
					Height_Rescaled,
					(char **)bitmap28x28);
		}
	}
}

void rotate_bitmap(char **bitmap28x28, char *bitmap28x28_rotated)
{
	int Width_Rescaled=28, Height_Rescaled=28;

	/* rotate image 90°, compensate LCD rotation => input for AI algo */
	for (int x=0; x<Width_Rescaled; x++) {
		for (int y=0; y<Height_Rescaled; y++) {
			bitmap28x28_rotated[y*Width_Rescaled + x] = bitmap28x28[x][y];
		}
	}
}

void printplot(char **bitmap28x28){
	int Width_Rescaled=28, Height_Rescaled=28;

	/* Draw rescaled image */
	for (int j =0; j < Width_Rescaled; j++)
	{
		for (int i= 0 ; i < Height_Rescaled ; i++)
		{
			if (bitmap28x28[i][j]==0x0)
			{
				BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
			}
			else BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

			BSP_LCD_FillCircle(i+10,j+10,1);
		}
	}
	/* Restore back the default color */
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
}

extern const void *              pSoundWav;

/** UART ***************************************/

ITStatus MsgReceived = RESET;

/**
 * @brief UART MSP Initialization
 *        This function configures the hardware resources used in this example:
 *           - Peripheral's clock enable
 *           - Peripheral's GPIO Configuration
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable GPIO TX/RX clock */
	USARTChapeau_TX_GPIO_CLK_ENABLE();
	USARTChapeau_RX_GPIO_CLK_ENABLE();


	/* Enable USARTx clock */
	USARTChapeau_CLK_ENABLE();

	/*##-2- Configure peripheral GPIO ##########################################*/
	/* UART TX GPIO pin configuration  */
	GPIO_InitStruct.Pin       = USARTChapeau_TX_PIN;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_PULLUP;
	GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Alternate = USARTChapeau_TX_AF;

	HAL_GPIO_Init(USARTChapeau_TX_GPIO_PORT, &GPIO_InitStruct);

	/* UART RX GPIO pin configuration  */
	GPIO_InitStruct.Pin = USARTChapeau_RX_PIN;
	GPIO_InitStruct.Alternate = USARTChapeau_RX_AF;

	HAL_GPIO_Init(USARTChapeau_RX_GPIO_PORT, &GPIO_InitStruct);

	HAL_NVIC_SetPriority(USARTChapeau_IRQn, 0, 1);
	HAL_NVIC_EnableIRQ(USARTChapeau_IRQn);
}

/**
 * @brief UART MSP De-Initialization
 *        This function frees the hardware resources used in this example:
 *          - Disable the Peripheral's clock
 *          - Revert GPIO configuration to their default state
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
	/*##-1- Reset peripherals ##################################################*/
	USARTChapeau_FORCE_RESET();
	USARTChapeau_RELEASE_RESET();

	/*##-2- Disable peripherals and GPIO Clocks #################################*/
	/* Configure USART6 Tx as alternate function  */
	HAL_GPIO_DeInit(USARTChapeau_TX_GPIO_PORT, USARTChapeau_TX_PIN);
	/* Configure USART6 Rx as alternate function  */
	HAL_GPIO_DeInit(USARTChapeau_RX_GPIO_PORT, USARTChapeau_RX_PIN);

	/*##-3- Disable the NVIC for UART ##########################################*/
	HAL_NVIC_DisableIRQ(USARTChapeau_IRQn);
}


/* UART test :
 *      Need a link between D0 and D1
 */
void initUART_Prepare_Rx(void)
{
	UartHandleChapeau.Instance        = USARTChapeau;

	UartHandleChapeau.Init.BaudRate   = 9600;
	UartHandleChapeau.Init.WordLength = UART_WORDLENGTH_8B;
	UartHandleChapeau.Init.StopBits   = UART_STOPBITS_1;
	UartHandleChapeau.Init.Parity     = UART_PARITY_NONE;
	UartHandleChapeau.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
	UartHandleChapeau.Init.Mode       = UART_MODE_TX_RX;
	UartHandleChapeau.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if(HAL_UART_DeInit(&UartHandleChapeau) != HAL_OK)
	{
		AVS_TRACE_ERROR("HAL_UART_DeInit failed");
	}
	if(HAL_UART_Init(&UartHandleChapeau) != HAL_OK)
	{
		AVS_TRACE_ERROR("HAL_UART_Init failed");
	}

	AVS_TRACE_INFO("Wait for message on UART (D0 pin)");
	memset(aRxBuffer, MSG_STUF_CHAR, RXBUFFERSIZE);
	MsgReceived = RESET;
	if(HAL_UART_Receive_IT(&UartHandleChapeau, (uint8_t *)aRxBuffer, RXBUFFERSIZE) != HAL_OK)
	{
		AVS_TRACE_ERROR("HAL_UART_Receive_IT failed");
	}

	AVS_TRACE_INFO("HAL_UART_Receive_DEBUG");

}

void SendMsgOnUART(const uint8_t * msg)
{
	uint8_t size;

	AVS_TRACE_INFO("Send message on UART (D1 pin): '%s'", msg);

	memset(aTxBuffer, MSG_STUF_CHAR, TXBUFFERSIZE);
	/* Copy msg into Tx buffer then add a special char at the end of msg */
	size = (strlen(msg)>TXBUFFERSIZE - 1)? TXBUFFERSIZE - 1 : strlen(msg);
	memcpy(aTxBuffer, msg, size);
	aTxBuffer[size] = MSG_END_CHAR;
	if(HAL_UART_Transmit(&UartHandleChapeau, aTxBuffer, TXBUFFERSIZE, 5000)!= HAL_OK)
	{
		AVS_TRACE_ERROR("HAL_UART_Transmit failed");
	}
}


void service_ChapeauUart_task(void  const * argument)
{
	/* you can use this thread to handle UART communication */
	AVS_TRACE_INFO("start Harry Potter UART thread, are you talking to me ?");

	initUART_Prepare_Rx();

	while (1) {

		if(MsgReceived == SET){
			AVS_TRACE_INFO("A message has been received on UART :");
			/* Create a null-terminated valid string from the received buffer */
			for(int i=0; i<RXBUFFERSIZE; i++)
				if (aRxBuffer[i] == MSG_END_CHAR)
					aRxBuffer[i] = 0;
			AVS_TRACE_INFO("     =>'%s'", aRxBuffer);

			switch(aRxBuffer)
			{
				case "stupefix":
				SendMsgOnUART((uint8_t *) "enervatum");
				break;

				case "expelliarmus":
				SendMsgOnUART((uint8_t *) "accio");
				break;

				case "amplificatum":
				SendMsgOnUART((uint8_t *) "reducto");
				break;

				case "collaporta":
				SendMsgOnUART((uint8_t *) "alohomora");
				break;

				case "nox":
				SendMsgOnUART((uint8_t *) "lumos");
				break;

				case "rictusempra":
				SendMsgOnUART((uint8_t *) "protego");
				break;

				case "sectumsempra":
				SendMsgOnUART((uint8_t *) "vulnera_sanetur");
				break;

				case "epouvantard":
				SendMsgOnUART((uint8_t *) "riddikulus");
				break;

				case "arania":
				SendMsgOnUART((uint8_t *) "arania_exumai");
				break;

				case "mangemort":
				SendMsgOnUART((uint8_t *) "expelliarmus");
				break;

				case "detraqueur":
				SendMsgOnUART((uint8_t *) "expecto_patronum");
				break;

				case "avada_kedavra":
				SendMsgOnUART((uint8_t *) "harry_potter");
				break;

				default:
				break;
			}

			// Display it
			BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			BSP_LCD_FillRect(200, 0, 600, 80);
			BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
			BSP_LCD_SetBackColor(LCD_COLOR_GREEN);
			BSP_LCD_SetFont(&Font24);
			BSP_LCD_DisplayStringAt(210, 40, (uint8_t *)aRxBuffer, RIGHT_MODE);

			// Restore text color to black (for symbol drawing)
			BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

			// Launch a new reception
			initUART_Prepare_Rx();
		}



		osDelay(10);
	}
}
/**********************************************/


/** LEDs ***************************************/

/* SPI handler declaration */
SPI_HandleTypeDef SpiHandle;

enum {
	TRANSFER_WAIT,
	TRANSFER_COMPLETE,
	TRANSFER_ERROR
};

enum {
	RED,
	GREEN,
	BLUE,
	YELLOW,
	PINK,
	WHITE
};

void service_ChapeauLed_task(void  const * argument)
{
	/* you can use this thread to handle LEDs */

	AVS_TRACE_INFO("start Harry Potter Led thread, and the light goes on");

	/* init code */

	/* Set the SPI parameters */
	SpiHandle.Instance               = SPI2;
	SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
	SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
	SpiHandle.Init.CLKPolarity       = SPI_POLARITY_HIGH;
	SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
	SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle.Init.CRCPolynomial     = 7;
	SpiHandle.Init.NSS               = SPI_NSS_SOFT;

	SpiHandle.Init.Mode = SPI_MODE_MASTER;

	HAL_SPI_MspInit(&SpiHandle);

	if(HAL_SPI_Init(&SpiHandle) != HAL_OK)
	{
		/* Initialization Error */
		//Error_Handler();
		AVS_TRACE_ERROR("HAL_SPI_INIT_ERROR !");
	}

	/* Buffer used for transmission */

	//uint8_t aTxBuffer[] = "****SPI - Two Boards communication based on DMA **** SPI Message ******** SPI Message ******** SPI Message ****";
    int activeLed = 8;
	uint8_t aTxBuffer[88] = {0};
	getBuffer(aTxBuffer, activeLed);

	/* transfer state */

	__IO uint32_t wTransferState = TRANSFER_WAIT;

	uint32_t ledTxTimeout = 2000;
	HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(&SpiHandle, (uint8_t*)aTxBuffer, BUFFERSIZE,ledTxTimeout);
	//HAL_StatusTypeDef halStatus = HAL_SPI_Transmit_IT(&SpiHandle, (uint8_t*)aTxBuffer, BUFFERSIZE);

	switch(halStatus){
	case HAL_OK:
		AVS_TRACE_ERROR("LED : HAL_SPI_TransmitReceive_OK !");
		break;
	case HAL_ERROR:
		AVS_TRACE_ERROR("LED : HAL_SPI_TransmitReceive_ERROR !");
		break;
	case HAL_BUSY:
		AVS_TRACE_ERROR("LED : HAL_SPI_TransmitReceive_BUSY !");
		break;
	case HAL_TIMEOUT:
		AVS_TRACE_ERROR("LED : HAL_SPI_TransmitReceive_TIMEOUT !");
		break;
	default:
		break;
	}

	/*##-3- Wait for the end of the transfer ###################################*/
	/*  Before starting a new communication transfer, you must wait the callback call
	        to get the transfer complete confirmation or an error detection.
	        For simplicity reasons, this example is just waiting till the end of the
	        transfer, but application may perform other tasks while transfer operation
	        is ongoing. */
	/*while (wTransferState == TRANSFER_WAIT)
	{
		osDelay(10);
	}

	switch(wTransferState)
	{
	case TRANSFER_COMPLETE :
		//##-4- Compare the sent and received buffers ##############################
		AVS_TRACE_INFO("HAL_SPI_TRANSFER_COMPLETE !");
		break;
	default :
		//Error_Handler();
		AVS_TRACE_ERROR("HAL_SPI_DEFAULT_ERROR !");
		break;
	}*/

	int waveDirection = 0; //0 - vers la gauche, 1 - vers la droite
	int ledDelay = 50;

	while (1) {
		/* loop. Don't forget to use osDelay to allow other tasks to be scedulled */

		for(int cl = 0; cl < 6; cl++){

			if(waveDirection == 0){
				for(int aL = 0 ; aL < 20 ; aL++){
					getBuffer(aTxBuffer, aL, cl);

					HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(&SpiHandle, (uint8_t*)aTxBuffer, BUFFERSIZE,ledTxTimeout);
					osDelay(ledDelay);
				}
				waveDirection = 1;
			}
			else{
				for(int aL = 19 ; aL >= 0 ; aL--){
					getBuffer(aTxBuffer, aL, cl);
					HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(&SpiHandle, (uint8_t*)aTxBuffer, BUFFERSIZE,ledTxTimeout);
					osDelay(ledDelay);
				}

				waveDirection = 0;
			}

		}


	}
}

void getBuffer(uint8_t *buf, int ledChoice, int colorChoice) {

	//Start Bits
	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;

	//LED Bits
	for(uint8_t ledn = 0 ; ledn < 20 ; ledn ++){
		int i = (ledn * 4) + 4;

		if(ledn == ledChoice){
			buf[i] = 0xEF;   //50% allumé
		}
		else{
			buf[i] = 0xE0;   //0%
		}

		switch(colorChoice){
		case 0: //RED
			buf[i+1] = 0x00; //Blue
			buf[i+2] = 0x00; //Green
			buf[i+3] = 0xFF; //Red
			break;
		case 1: //GREEN
			buf[i+1] = 0x00; //Blue
			buf[i+2] = 0xFF; //Green
			buf[i+3] = 0x00; //Red
			break;
		case 2: //BLUE
			buf[i+1] = 0xFF; //Blue
			buf[i+2] = 0x00; //Green
			buf[i+3] = 0x00; //Red
			break;
		case 3: //YELLOW
			buf[i+1] = 0xFF; //Blue
			buf[i+2] = 0x00; //Green
			buf[i+3] = 0xFF; //Red
			break;
		case 4: //PINK
			buf[i+1] = 0xD7; //Blue
			buf[i+2] = 0x9E; //Green
			buf[i+3] = 0xF4; //Red
			break;
		case 5: //WHITE
			buf[i+1] = 0xFF; //Blue
			buf[i+2] = 0xFF; //Green
			buf[i+3] = 0xFF; //Red
			break;
		default:
			buf[i+1] = 0x00; //Blue
			buf[i+2] = 0x00; //Green
			buf[i+3] = 0x00; //Red
			break;
		}

	}

	//End Bits
	buf[84] = 0xFF;
	buf[85] = 0xFF;
	buf[86] = 0xFF;
	buf[87] = 0xFF;
}

static void Error_Handler(void)
{
	/* Configure LED1 which is shared with SPI2_SCK signal */
	BSP_LED_Init(LED1);
	BSP_LED_Off(LED1);
	AVS_TRACE_ERROR("Erreur Handler du LED");
	while(1)
	{
		/* Toggle LED1 for error */
		BSP_LED_Toggle(LED1);
		HAL_Delay(1000);
	}
}

static uint16_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength)
{
	while (BufferLength--)
	{
		if((*pBuffer1) != *pBuffer2)
		{
			return BufferLength;
		}
		pBuffer1++;
		pBuffer2++;
	}

	return 0;
}


/**********************************************/
#define LCD_COLOR_FLOPPY           ((uint32_t) 0xFFFF0066)

/** User Interface *******************************/

void redraw(){
	/* Clear the LCD */
	BSP_LCD_Clear(LCD_COLOR_WHITE);

	//****************************************************************
	// Top panel
	//****************************************************************

	// "Title" box
	BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
	BSP_LCD_FillRect(0, 0, 200, 80);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_DARKBLUE);
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_DisplayStringAt(10, 40, (uint8_t *)"ST Team", LEFT_MODE);


	// Display it
	BSP_LCD_SetTextColor(LCD_COLOR_FLOPPY);
	BSP_LCD_FillRect(200, 0, 600, 40);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_FLOPPY);
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_DisplayStringAt(210, 15, (uint8_t *)"Floppy Coders", RIGHT_MODE);

	// Display it
	BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
	BSP_LCD_FillRect(200, 40, 600, 40);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_GREEN);
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_DisplayStringAt(210, 55, (uint8_t *)"wait msg", RIGHT_MODE);

	//****************************************************************
	// Left panel
	//****************************************************************

	// "Alexa" box
	BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
	BSP_LCD_FillRect(0, 80, 200, 100);
	BSP_LCD_SetBackColor(LCD_COLOR_CYAN);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_DisplayStringAt(10, 130, (uint8_t *)"Alexa", LEFT_MODE);

	// "Send" box
	BSP_LCD_SetTextColor(LCD_COLOR_LIGHTBLUE);
	BSP_LCD_FillRect(0, 180, 200, 100);
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTBLUE);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_DisplayStringAt(10, 230, (uint8_t *)"Send", LEFT_MODE);

	// "Clear" box
	BSP_LCD_SetTextColor(LCD_COLOR_LIGHTBLUE);
	BSP_LCD_FillRect(0, 380, 200, 100);
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTBLUE);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_DisplayStringAt(10, 430, (uint8_t *)"Clear", LEFT_MODE);


	//****************************************************************
	// Right panel
	//****************************************************************
	// ...

	// Lines
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_DrawLine(0, 80, 800, 80);
	BSP_LCD_DrawLine(200, 0, 200, 480);
	BSP_LCD_DrawLine(600, 80, 600, 480);
	BSP_LCD_DrawLine(0, 180, 200, 180);
	BSP_LCD_DrawLine(0, 280, 200, 280);
	BSP_LCD_DrawLine(0, 380, 200, 380);

	//DEMARRAGE MON TOURNOI DES SORCIERS
	BSP_LCD_SetTextColor(LCD_COLOR_DARKGRAY);
	BSP_LCD_FillRect(600, 80, 200, 100);
	BSP_LCD_SetBackColor(LCD_COLOR_DARKGRAY);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_DisplayStringAt(600, 130, (uint8_t *)"Lancer TDS", LEFT_MODE);

}


void service_Chapeau_task(void  const * argument)
{
	uint16_t x1, y1;
	uint16_t compteur = 0;
	uint8_t AVSrunning = 0;
	int Symbole;
	image* imageNew;
	uint16_t points_nb;
	char **bitmap;
	char *bitmap_rotated;

	AVS_TRACE_INFO("start Harry Potter thread, welcome in magic school !");

	BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);

	/* Touchscreen initialization */
	if (BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize()) == TS_ERROR) {
		AVS_TRACE_INFO("BSP_TS_Init error\n");
	}
	redraw();
	resetTouchInfos();

	int n_retry = NB_RETRY_RELEASE_DETECTION;
	int point_series_on_going = 0;
	int release_nb = 0;

	imageNew = malloc(sizeof(image));
	if(!imageNew)
		AVS_TRACE_ERROR("Fails to allocate image");

	bitmap = (char **)malloc(28 * sizeof(char *));
	if(!bitmap)
		AVS_TRACE_ERROR("Fails to allocate bitmap");

	for (int i=0; i<28; i++) {
		bitmap[i] = (char *)malloc(28 * sizeof(char));
		if(!bitmap[i])
			AVS_TRACE_ERROR("Fails to allocate bitmap column");
	}

	bitmap_rotated = (char *)malloc(28 * 28 * sizeof(char *));
	if(!bitmap_rotated)
		AVS_TRACE_ERROR("Fails to allocate bitmap_rotated");

	memset(imageNew, 0, sizeof(image));

	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

	while (1) {

		BSP_TS_GetState(&TS_State);
		/* if no touch detected, check for release state */
		if(!TS_State.touchDetected && point_series_on_going) {
			if (--n_retry == 0) {
				point_series_on_going = 0;
				AVS_TRACE_INFO("Release", release_nb);
				n_retry = NB_RETRY_RELEASE_DETECTION;
			}
		}

		if(TS_State.touchDetected) {
			n_retry = NB_RETRY_RELEASE_DETECTION;
			/* One or dual touch have been detected          */

			/* Get X and Y position of the first touch post calibrated */
			x1 = TS_State.touchX[0];
			y1 = TS_State.touchY[0];

			//AVS_TRACE_INFO("Touch Detected x=%d y=%d  %d\n", x1, y1, compteur);

			if ((x1 > 0) && ( x1 < 200) && (y1 > 80) && ( y1 < 480)){
				// Left panel
				if ((x1 > 0) && ( x1 < 200) && (y1 > 80) && ( y1 < 180))
				{
					AVS_TRACE_INFO("Touch detected : 'Alexa' button\n");

					AVS_Play_Sound(hInstance, AVS_PLAYSOUND_PLAY, (void *)(uint32_t)pSoundWav, 100);

					/* Starts the capture only if northing occurs on the system ( ie BUSY etc...) */
					if(AVS_Set_State(hInstance, AVS_STATE_START_CAPTURE) == AVS_OK){
						/* Capture started and button pressed */
						//buttonState = TRUE;
					}

					osDelay(100); // to avoid multiple detections
				} else if ((x1 > 0) && ( x1 < 200) && (y1 > 180) && ( y1 < 280))
				{
					AVS_TRACE_INFO("Touch detected : 'Send' button\n");

					/* Save only if point series is finishedn, users has released the touch
					 * before pressing the Save area */
					if (point_series_on_going == 1)
						continue;

					/* Print only if at least a touch was detected */
					if(imageNew->series_nb) {

						create_bitmap(imageNew, bitmap);
						rotate_bitmap(bitmap, (char *)bitmap_rotated);
					}

					memset(imageNew, 0, sizeof(image));

					Symbole = ai_Predict(bitmap_rotated);

					switch(Symbole)
					{
						case 0:
							SendMsgOnUART((uint8_t *) "stupefix");
							break;

						case 1:
							SendMsgOnUART((uint8_t *)"expelliarmus");
							break;

						case 2:
							SendMsgOnUART((uint8_t *) "amplificatum");
							break;

						case 3:
							SendMsgOnUART((uint8_t *) "collaporta");
							break;

						case 4:
							SendMsgOnUART((uint8_t *) "nox");
							break;

						case 5:
							SendMsgOnUART((uint8_t *) "rictusempra");
							break;

						case 6:
							SendMsgOnUART((uint8_t *) "sectumsempra");
							break;

						case 7:
							SendMsgOnUART((uint8_t *) "epouvantard");
							break;

						case 8:
							SendMsgOnUART((uint8_t *) "arania");
							break;

						case 9:
							SendMsgOnUART((uint8_t *) "mangemort");
							break;

						case 10:
							SendMsgOnUART((uint8_t *) "detraqueur");
							break;

						case 11:
							SendMsgOnUART((uint8_t *) "avada_kedavra");
							break;

						default:
							break;
					}

					AVS_TRACE_INFO("%d",Symbole);

					point_series_on_going = 0;
					resetTouchInfos();

					// redraw the full UI and draw the last drawn symbol
					redraw();
					printplot(bitmap);

					osDelay(100); // to avoid multiple detections

				} else if ((x1 > 0) && ( x1 < 200) && (y1 > 380) && ( y1 < 480))
				{
					AVS_TRACE_INFO("Touch detected : 'Clear' button\n");

					/* Clear only if point series is finished, users has released the touch
					 * before pressing the Save area */
					if (point_series_on_going == 1)
						continue;

					redraw();
					BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

					memset(imageNew, 0, sizeof(image));
					point_series_on_going = 0;

					osDelay(100); // to avoid multiple detections

				}
			} else if ((x1 > 200) && ( x1 < 600) && (y1 > 80) && ( y1 < 480))
			{
				//AVS_TRACE_INFO("Touch detected : 'Middle' button\n");
				if(!point_series_on_going) {
					/* Create new point series */
					point_series_on_going = 1;
					imageNew->series_nb++;
					if (imageNew->series_nb > MAX_SERIES) {
						AVS_TRACE_INFO("MAX series reached, clear Image");
						memset(imageNew, 0, sizeof(image));
						point_series_on_going = 0;
					}
				}

				/* Get X and Y position of the first touch post calibrated */
				imageNew->point_series[imageNew->series_nb-1].points_nb++;
				points_nb = imageNew->point_series[imageNew->series_nb-1].points_nb;
				imageNew->point_series[imageNew->series_nb-1].points[points_nb-1].x = x1;
				imageNew->point_series[imageNew->series_nb-1].points[points_nb-1].y = y1;

				/* if we reach the maximum pointpoint_series_on_goings number of a serie, create a new one */
				if(points_nb >= MAX_POINTS) {
					point_series_on_going = 0;
					AVS_TRACE_INFO(" Max Points reached, create new serie");
				}

				if (x1>maxX) maxX= x1;
				if (y1>maxY) maxY= y1;
				if (x1<minX) minX= x1;
				if (y1<minY) minY = y1;
				//AVS_TRACE_INFO("max x=%d y=%d\n", maxX, maxY);
				//AVS_TRACE_INFO("min x=%d y=%d\n", minX, minY);
				BSP_LCD_FillCircle(x1, y1, 10);

			}else if ((x1 > 600) && ( x1 < 800) && (y1 > 80) && ( y1 < 480)){
				//RIGHT PANEL
				if ((x1 > 600) && ( x1 < 800) && (y1 > 80) && ( y1 < 180)){
					// DEMARRE TDS
					AVS_TRACE_INFO("Touch detected : 'DEMARRE TDS' button\n");

					AVS_Play_Sound(hInstance, AVS_PLAYSOUND_PLAY, (void *)(uint32_t)pSoundWav, 100);
					osDelay(100); // to avoid multiple detections
				}
			}
			osDelay(10);
		}else {
			osDelay(5);
		}
	}
	free(imageNew);
	for (int i=0; i<28; i++)
		free(bitmap[i]);
	free(bitmap);
	free(bitmap_rotated);
}
