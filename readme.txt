/**
  @page STVS4A application

  @verbatim
  ******************************************************************************
  * @file    readme.txt
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    16-March-2018
  * @brief   Description of ST AVS application.
  ******************************************************************************
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  @endverbatim

@par Application Description

The sample application implements an Alexa client for 32F769IDISCOVERY board linked through ethernet or WIFI (with ESP-01 8266 add on) to Amazon cloud.

Please refer to STVS4A user manual (available on st.com) to get more detailed information.
Information on the SDK and wifi setup can also be found in Middlewares\ST\STVS4A\Docs\reference_manual_stavs.html

@par Hardware and Software environment

  - MCU board: 32F769IDISCOVERY (with STM32F769 rev Z)
  
  - Internet connection must access to Internet without any proxy
      * The connection to Amazon cloud require an authentication phase. 
      * To proceed, connect a web browser on the same sub-network as the STM32 board.
      * Typically, both connect to a router

  - Authentication: 
      * connect your PC's or smartphone's browser to http://xxx.xxx.xxx.xxx where xxx.xxx.xxx.xxx is the IP address of the board.
      * This address is written on the screen after initialization.
      * When redirected to Amazon server, enter your Amazon credentials
      * then, the browser will try to reach an URL starting with http://stavs?code=... 
      * replace "stavs" by the IP address (and keep the rest of the line unchanged)
      * The board will get a token , and store some info in flash so authentication procedure will not be needed after next reboot.


@par How to use it ?

  - When the initialization is done, and assuming that authentication has been done at least once...

      * The screens displays "Idle" when ready for interaction. 
      * If authentication has not been once (see previous chapter), it displays "restart"

      * Push the blue button for half a second
      * The board outputs a sound (a headset or external speaker must have been plugged to the green connector)
      * Ask your question / request to Alexa

  - The application supports interactions with Alexa with questions like:
      * “Tell me a joke”, “When was Harry Potter published?”
      * "Set an alarm for 7:00 AM!", “Cancel alarm”, ”Set an alarm”
      * "Volume up", "Set volume to 3"

  - The Graphical User Interface (GUI) integrates several pages. 
      * A button on the right bottom can be pushed to scroll from one page to next one.
      * One page is used to select the language or variant used when interacting with Alexa.

  - To get more information, please refer to the user manual UM2354, available on st.com.
      * UM2354: Alexa Voice Service software expansion for STM32Cube




Application build and flash

  - The application can support both configuration of flash.
      * Single bank mode (Option byte NDBANK = 1) is better supported by some tools.
      * The value of the option byte can be checked / modified with ST-Link utility of STM32 Cube Programmer.

  - The application embeds an endurance test suite. 
      * These tests need some data that are stored in the external QSPI Flash.
      * A specific build is required to generate the binary containing the assets in QSPI 
            .EWARM and SW2STM32 : 
                .unset the AVS_USE_QSPI_FLASHED flag 
            .MDK-ARM : 
                .unset the AVS_USE_QSPI_FLASHED flag
                .set the  AVS_CREATE_QSPI_SECTION  flag in assetConfig.h (this flag is used in .sct file for section definition)
      * In such a case, the debugger does not program it and the executable file 
      * needs to be programmed through ST-LINK utility by means of the following sequence of operations:
      * 1.Erase chip command in the Target menu
      * 2. Open File command in the File menu
      * 3. Program & Verify command in the Target menu
      * Note: If the test assets have not been programmed, the ‘Test start’ button does not work.
      * Note: The tests are done in english (QSPI flash embeds audio files in english language).


  - A pre-built binary is located in the folder: Projects/STM32F769I-Discovery/Application/STVS4A/Binaries
      * This binary file embeds the QSPI content for endurance tests


  - WARNING: before opening the project with any toolchain be sure your folder
    installation path is not too in-depth since the toolchain may report errors
    after building.

  - Open and build the project with one of the supported development toolchains (see the release note
    for detailed information about the version requirements). 


@par Directory contents


---> The application is located in Projects/STM32F769I-Discovery/Application/STVS4A 

The sub folders contain different kind of files.
Binaries: contain a pre-built executable
Config  : include files for component configuration
EWARM   : files for IAR project
MDK-ARM : files for Keil project
SW4STM32: files for System Workbench project
inc     : project include files
Src     : project source files

The source project files are:
clean_lcd_swap_buffer.c : LCD display
ethernetif.c            : link between ethernet HAL and LWIP
main.c                  : application C main file
platform_init.c         : hardware platform initialization
stm32f7xx_it.c          : interrupt handler initialization
system_stm32f7xx.c      : part of MCU initialization
usbd_conf_f7.c          : helper for audio in/out debug. It is not used by default in the application


 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
