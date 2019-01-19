/**
******************************************************************************
* @file    platform_adapt.c
* @author  MCD Application Team
* @version V1.0.0
* @date    04-Aout-2017
* @brief   Board HW overload
******************************************************************************
* @attention
*
* <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
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

#include "platform_init.h"
#include "cmsis_os.h"
#include "avs.h"
#include "service.h"

/*

    Check the Touch screen and returns state and position 

*/


void TS_Get_State(TS_StateTypeDef      *pTsState, uint32_t *touchDetected,uint32_t *touchX,uint32_t *touchY)
{
  AVS_ASSERT(pTsState);
  AVS_ASSERT(touchDetected);
  AVS_ASSERT(touchX);
  AVS_ASSERT(touchY);
  BSP_TS_GetState(pTsState);
  *touchDetected = pTsState->touchDetected;
  *touchX = pTsState->touchX[0];
  *touchY = pTsState->touchY[0];
  
}



/* ----------------------------------

    avs srvice persistent  storage   adaptation 
    the flash management varies from a board to another 
    an app must overload service_persistent_erase_storage & service_persistent_erase_storage
    to support the persistant storage 
  
---------------------------------- */
static uint32_t persistent_offset=0x081E0000;
static uint32_t persistent_sector=FLASH_SECTOR_23;
static uint32_t persistent_sector_size=128;



void platform_config_persistent_storage(uint32_t Offset,uint32_t Sector,uint32_t SectorSize)
{
  persistent_offset = Offset;
  persistent_sector = Sector;
  persistent_sector_size = SectorSize;

}

void Get_Board_Flash_Storage(uint32_t *pOffset,uint32_t *pSector,uint32_t *pSectorSize)
{

/* Other boards  may need to override this function*/
 if(pOffset != 0)
  {
     *pOffset = persistent_offset;
  }
  if(pSector != 0)
  {
     *pSector = persistent_sector;
  }
  if(pSectorSize != 0)
  {
     *pSectorSize = persistent_sector_size;
  }
}



/*
    
    Check if the flash is empty after erasing 

*/


static AVS_Result check_empty(uint8_t *pByte,uint32_t size)
{
  
  for(int32_t a = 0; a < size; a++)
  {
    if(pByte[a] != 0xFF)
    {
      AVS_TRACE_ERROR("Check  flash not erased : %08x = %x", &pByte[a], pByte[a]);
      return AVS_ERROR;
    }
  }
  return AVS_OK;
}

/*
    Erase the persitent sector 
    
*/

AVS_Result  service_persistent_erase_storage(void)
{
  FLASH_EraseInitTypeDef EraseInitStruct = {0};
  uint32_t SectorError = 0;
  
  uint32_t     flash_base_adress ;
  uint32_t     flash_base_sector ;
  uint32_t     flash_sector_size ;
   
  Get_Board_Flash_Storage(&flash_base_adress, &flash_base_sector,&flash_sector_size);
  
  
  uint32_t EraseTryCount=10;
  /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */   
  
  
  while(EraseTryCount) 
  {
    EraseInitStruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector       = flash_base_sector;
    EraseInitStruct.Banks        = FLASH_BANK_2;
    EraseInitStruct.NbSectors    = 1;
    
    
    
    
    HAL_FLASH_Unlock();
    SCB_InvalidateDCache();
    HAL_StatusTypeDef result = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
    HAL_FLASH_Lock();
    if(result == HAL_OK)  
    {
      if(check_empty((uint8_t *)flash_base_adress,flash_sector_size*1024)==AVS_OK)
      {
        break;
      }
    }
    EraseTryCount--;
    AVS_TRACE_INFO("Re-try to erase the flash");
  }
  
  if(EraseTryCount ==0)
  {
      AVS_TRACE_ERROR("Cannot erase the flash");
      return AVS_ERROR;
  }
  return AVS_OK;  
}



/**
 * @brief Write token from flash
 * @note  BE CAREFULL :the sector number is hard coded below
 *        and this address must be in lin with scatter file content
 * @param[in] context Pointer to the AVS context
 * @return Error code
 **/





AVS_Result service_persistent_write_storage(Persistant_t *pPersistent)
{
 
  uint32_t Address ;
  uint64_t Data;
  pPersistent->signature = PERSISTENT_SIGNATURE;
  uint32_t     flash_base_adress ;
  Get_Board_Flash_Storage(&flash_base_adress, 0,0);
  

  /* check if the content changes (if not, no need to flash the same content) */
  uint32_t     data_has_changed = 0;
  for (int32_t i = 0; i < sizeof(*pPersistent); i++)
  {
    if ( ((uint8_t*)pPersistent)[i] != ((uint8_t*)flash_base_adress)[i] )
    {
      data_has_changed = 1;
    }
  }
  if (data_has_changed == 0)
  {
    AVS_TRACE_DEBUG("no need to overwrite flash memory");
    return  AVS_OK;
  }
  
  
  AVS_TRACE_DEBUG("going to overwrite flash memory");

  /* Unlock the flash */
  HAL_StatusTypeDef status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    AVS_TRACE_ERROR("HAL_FLASH_Unlock failed :%d", status);
    return AVS_ERROR;
  }
  
  if(service_persistent_erase_storage() != AVS_OK) 
  {
    return AVS_ERROR;
  }

  uint32_t *pStorage = (uint32_t *)pPersistent;
  uint32_t szStorage = (sizeof(*pPersistent)/4) + 1;
  

  /* Write the storage in the flash sector */
  HAL_FLASH_Unlock();
  
  AVS_Result ret = AVS_OK;
  for (int32_t i = 0; i < szStorage; i++)
  {
    Address = flash_base_adress + 4 * i;
    Data = pStorage[i];
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, Data) != HAL_OK)
    {
      ret = AVS_ERROR;
      break;
    }
  }
  HAL_FLASH_Lock();
  if(ret != AVS_OK)
  {
     AVS_TRACE_ERROR("HAL_FLASH_Program failed while writing at 0x%x", Address);
     return AVS_ERROR;
  }
  
  if(memcmp((const void *)(uint32_t)flash_base_adress,(const void *)(uint32_t)pStorage,szStorage*sizeof(uint32_t)) != 0)
  {
    AVS_TRACE_ERROR("Write Persistent: Verify error");
    return AVS_ERROR;
    
  }

  AVS_TRACE_DEBUG("Persistent: Refresh OK");
  return  AVS_OK;
}
