/*******************************************************************************
  System Initialization File

  File Name:
    initialization.c

  Summary:
    This file contains source code necessary to initialize the system.

  Description:
    This file contains source code necessary to initialize the system.  It
    implements the "SYS_Initialize" function, defines the configuration bits,
    and allocates any necessary global system resources,
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "configuration.h"
#include "definitions.h"


// ****************************************************************************
// ****************************************************************************
// Section: Configuration Bits
// ****************************************************************************
// ****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Driver Initialization Data
// *****************************************************************************
// *****************************************************************************
// <editor-fold defaultstate="collapsed" desc="DRV_MEMORY Instance 0 Initialization Data">

uint8_t gDrvMemory0EraseBuffer[DRV_SST26_ERASE_BUFFER_SIZE] __attribute__((aligned(32)));

DRV_MEMORY_CLIENT_OBJECT gDrvMemory0ClientObject[DRV_MEMORY_CLIENTS_NUMBER_IDX0] = { 0 };

DRV_MEMORY_BUFFER_OBJECT gDrvMemory0BufferObject[DRV_MEMORY_BUFFER_QUEUE_SIZE_IDX0] = { 0 };

const MEMORY_DEVICE_API drvMemory0DeviceAPI = {
    .Open               = DRV_SST26_Open,
    .Close              = DRV_SST26_Close,
    .Status             = DRV_SST26_Status,
    .SectorErase        = DRV_SST26_SectorErase,
    .Read               = DRV_SST26_Read,
    .PageWrite          = DRV_SST26_PageWrite,
    .EventHandlerSet    = NULL,
    .GeometryGet        = (GEOMETRY_GET)DRV_SST26_GeometryGet,
    .TransferStatusGet  = (TRANSFER_STATUS_GET)DRV_SST26_TransferStatusGet
};

const DRV_MEMORY_INIT drvMemory0InitData =
{
    .memDevIndex                = DRV_SST26_INDEX,
    .memoryDevice               = &drvMemory0DeviceAPI,
    .isMemDevInterruptEnabled   = false,
    .isFsEnabled                = false,
    .ewBuffer                   = &gDrvMemory0EraseBuffer[0],
    .clientObjPool              = (uintptr_t)&gDrvMemory0ClientObject[0],
    .bufferObj                  = (uintptr_t)&gDrvMemory0BufferObject[0],
    .queueSize                  = DRV_MEMORY_BUFFER_QUEUE_SIZE_IDX0,
    .nClientsMax                = DRV_MEMORY_CLIENTS_NUMBER_IDX0
};

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="DRV_MEMORY Instance 1 Initialization Data">

uint8_t gDrvMemory1EraseBuffer[EFC_ERASE_BUFFER_SIZE] __attribute__((aligned(32)));

DRV_MEMORY_CLIENT_OBJECT gDrvMemory1ClientObject[DRV_MEMORY_CLIENTS_NUMBER_IDX1] = { 0 };

DRV_MEMORY_BUFFER_OBJECT gDrvMemory1BufferObject[DRV_MEMORY_BUFFER_QUEUE_SIZE_IDX1] = { 0 };

const MEMORY_DEVICE_API drvMemory1DeviceAPI = {
    .Open               = DRV_EFC_Open,
    .Close              = DRV_EFC_Close,
    .Status             = DRV_EFC_Status,
    .SectorErase        = DRV_EFC_SectorErase,
    .Read               = DRV_EFC_Read,
    .PageWrite          = DRV_EFC_PageWrite,
    .EventHandlerSet    = (EVENT_HANDLER_SET)DRV_EFC_EventHandlerSet,
    .GeometryGet        = (GEOMETRY_GET)DRV_EFC_GeometryGet,
    .TransferStatusGet  = (TRANSFER_STATUS_GET)DRV_EFC_TransferStatusGet
};

const DRV_MEMORY_INIT drvMemory1InitData =
{
    .memDevIndex                = 0,
    .memoryDevice               = &drvMemory1DeviceAPI,
    .isMemDevInterruptEnabled   = true,
    .isFsEnabled                = false,
    .ewBuffer                   = &gDrvMemory1EraseBuffer[0],
    .clientObjPool              = (uintptr_t)&gDrvMemory1ClientObject[0],
    .bufferObj                  = (uintptr_t)&gDrvMemory1BufferObject[0],
    .queueSize                  = DRV_MEMORY_BUFFER_QUEUE_SIZE_IDX1,
    .nClientsMax                = DRV_MEMORY_CLIENTS_NUMBER_IDX1
};

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="DRV_SST26 Initialization Data">

const SST26_PLIB_API drvSST26PlibAPI = {
    .CommandWrite  = QSPI_CommandWrite,
    .RegisterRead  = QSPI_RegisterRead,
    .RegisterWrite = QSPI_RegisterWrite,
    .MemoryRead    = QSPI_MemoryRead,
    .MemoryWrite   = QSPI_MemoryWrite
};

const DRV_SST26_INIT drvSST26InitData =
{
    .sst26Plib         = &drvSST26PlibAPI,
};

// </editor-fold>


// *****************************************************************************
// *****************************************************************************
// Section: System Data
// *****************************************************************************
// *****************************************************************************
/* Structure to hold the object handles for the modules in the system. */
SYSTEM_OBJECTS sysObj;
// *****************************************************************************
// *****************************************************************************
// Section: Library/Stack Initialization Data
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: System Initialization
// *****************************************************************************
// *****************************************************************************


/*******************************************************************************
  Function:
    void SYS_Initialize ( void *data )

  Summary:
    Initializes the board, services, drivers, application and other modules.

  Remarks:
 */

void SYS_Initialize ( void* data )
{
    CLK_Initialize();
	PIO_Initialize();

    NVIC_Initialize();
	SYSTICK_TimerInitialize();
	RSWDT_REGS->RSWDT_MR = RSWDT_MR_WDDIS_Msk;	// Disable RSWDT 

	WDT_REGS->WDT_MR = WDT_MR_WDDIS_Msk; 		// Disable WDT 

	BSP_Initialize();
    QSPI_Initialize();


    sysObj.drvMemory0 = DRV_MEMORY_Initialize((SYS_MODULE_INDEX)DRV_MEMORY_INDEX_0, (SYS_MODULE_INIT *)&drvMemory0InitData);

    sysObj.drvMemory1 = DRV_MEMORY_Initialize((SYS_MODULE_INDEX)DRV_MEMORY_INDEX_1, (SYS_MODULE_INIT *)&drvMemory1InitData);

    sysObj.drvSST26 = DRV_SST26_Initialize((SYS_MODULE_INDEX)DRV_SST26_INDEX, (SYS_MODULE_INIT *)&drvSST26InitData);




    APP_SST26_Initialize();
    APP_NVM_Initialize();
    APP_MONITOR_Initialize();


}


/*******************************************************************************
 End of File
*/
