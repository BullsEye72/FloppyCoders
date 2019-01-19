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

/** UART  ***************************************/

void service_ChapeauUart_task(void  const * argument)
{
    /* you can use this thread to handle UART communication */
	AVS_TRACE_INFO("start Harry Potter UART thread, are you talking to me ?");

	UART_HandleTypeDef UartHandle;
	__IO ITStatus UartReady = RESET;
	__IO uint32_t UserButtonStatus = 0;

	uint8_t aTxBuffer[] = " ****UART_TwoBoards communication based on DMA****  ****UART_TwoBoards communication based on DMA****  ****UART_TwoBoards communication based on DMA**** ";
	uint8_t aRxBuffer[100];

	  HAL_Init();

	  BSP_LED_Init(LED1);
	  BSP_LED_Init(LED2);

	  /*##-1- Configure the UART peripheral ######################################*/
	    /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
	    /* UART configured as follows:
	        - Word Length = 8 Bits
	        - Stop Bit = One Stop bit
	        - Parity = None
	        - BaudRate = 9600 baud
	        - Hardware flow control disabled (RTS and CTS signals) */
	    UartHandle.Instance        = USART6;

	    UartHandle.Init.BaudRate   = 9600;
	    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
	    UartHandle.Init.StopBits   = UART_STOPBITS_1;
	    UartHandle.Init.Parity     = UART_PARITY_NONE;
	    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
	    UartHandle.Init.Mode       = UART_MODE_TX_RX;

	    if(HAL_UART_DeInit(&UartHandle) != HAL_OK)
	    {
	      //Error_Handler();
	    	AVS_TRACE_INFO("DeInit_SUCCEDED !");
	    }
	    if(HAL_UART_Init(&UartHandle) != HAL_OK)
	    {
	      //Error_Handler();
	    	AVS_TRACE_INFO("Init_SUCCEDED !");
	    }

#ifdef TRANSMITTER_BOARD

	    BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

	    BSP_LED_Off(LED1);

	    if(HAL_UART_Receive_DMA(&UartHandle, (uint8_t *)aRxBuffer, 100) != HAL_OK)
	    {
	      //Error_Handler();
	    	AVS_TRACE_INFO("HAL_UART_Receive_DMA_SUCCEDED !");
	    }

	    if(HAL_UART_Transmit_DMA(&UartHandle, (uint8_t*)aTxBuffer, 100)!= HAL_OK)
	    {
	      //Error_Handler();
	    	AVS_TRACE_INFO("HAL_UART_Transmit_DMA_SUCCEDED !");
	    }

	    /* Reset transmission flag */
	    UartReady = RESET;

#else
	    /* The board receives the message and sends it back */

	      if(HAL_UART_Receive_DMA(&UartHandle, (uint8_t *)aRxBuffer, 100) != HAL_OK)
	      {
	        //Error_Handler();
	    	  AVS_TRACE_INFO("HAL_UART_Receive_DMA_SUCCEDED !");
	      }

	      /*while (UartReady != SET)
	      {
	          BSP_LED_On(LED1);
	          HAL_Delay(100);
	          BSP_LED_Off(LED1);
	          HAL_Delay(100);
	          BSP_LED_On(LED1);
	          HAL_Delay(100);
	          BSP_LED_Off(LED1);
	          HAL_Delay(500);
	      }*/

	      /* Reset transmission flag */
	      UartReady = RESET;
	      BSP_LED_Off(LED1);


	      if(HAL_UART_Transmit_DMA(&UartHandle, (uint8_t*)aTxBuffer, 100)!= HAL_OK)
	      {
	        //Error_Handler();
	    	  AVS_TRACE_INFO("HAL_UART_Transmit_DMA_SUCCEDED !");
	      }

#endif /* TRANSMITTER_BOARD */
    
    while (1) {
        /* loop. Don't forget to use osDelay to allow other tasks to be scedulled */
        osDelay(10);
    }
}
/**********************************************/


/** LEDs ***************************************/

void service_ChapeauLed_task(void  const * argument)
{
    /* you can use this thread to handle LEDs */

    AVS_TRACE_INFO("start Harry Potter Led thread, and the light goes on");

    /* init code */
    
    while (1) {
        /* loop. Don't forget to use osDelay to allow other tasks to be scedulled */
        osDelay(10);
    }
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

}


void service_Chapeau_task(void  const * argument)
{
    uint16_t x1, y1;
    uint16_t compteur = 0;
    uint8_t AVSrunning = 0;
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





