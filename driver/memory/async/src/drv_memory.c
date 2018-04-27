/******************************************************************************
  MEMORY Driver Interface Implementation

  Company:
    Microchip Technology Inc.

  File Name:
    drv_memory.c

  Summary:
    MEMORY Driver Interface Definition

  Description:
    The MEMORY Driver provides a interface to access the MEMORY on the PIC32
    microcontroller. This file implements the MEMORY Driver interface. This file
    should be included in the project if MEMORY driver functionality is needed.
*******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2016 - 2017 released Microchip Technology Inc. All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
*******************************************************************************/
//DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Include Files
// *****************************************************************************
// *****************************************************************************

#include "driver/memory/src/drv_memory_local.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global objects
// *****************************************************************************
// *****************************************************************************
#define SYS_DEBUG_PRINT(x,y)

/*************************************************
 * Hardware instance objects
 *************************************************/

DRV_MEMORY_OBJECT gDrvMemoryObj[DRV_MEMORY_INSTANCES_NUMBER];


/************************************************
 * This token is incremented for every request added to the queue and is used
 * to generate a different buffer handle for every request.
 ***********************************************/

static MEMORY_DEVICE_TRANSFER_STATUS DRV_MEMORY_HandleRead
(
    DRV_MEMORY_OBJECT *dObj,
    uint8_t *data,
    uint32_t blockAddress,
    uint32_t nBlocks
);

static MEMORY_DEVICE_TRANSFER_STATUS DRV_MEMORY_HandleWrite
(
    DRV_MEMORY_OBJECT *dObj,
    uint8_t *data,
    uint32_t blockAddress,
    uint32_t nBlocks
);

static MEMORY_DEVICE_TRANSFER_STATUS DRV_MEMORY_HandleErase
(
    DRV_MEMORY_OBJECT *dObj,
    uint8_t *data,
    uint32_t blockAddress,
    uint32_t nBlocks
);
static MEMORY_DEVICE_TRANSFER_STATUS DRV_MEMORY_HandleEraseWrite
(
    DRV_MEMORY_OBJECT *dObj,
    uint8_t *data,
    uint32_t blockAddress,
    uint32_t nBlocks
);

static const DRV_MEMORY_TransferOperation gMemoryXferFuncPtr[4] =
{
    DRV_MEMORY_HandleRead,
    DRV_MEMORY_HandleWrite,
    DRV_MEMORY_HandleErase,
    DRV_MEMORY_HandleEraseWrite,
};

// *****************************************************************************
// *****************************************************************************
// Section: MEMORY Driver Local Functions
// *****************************************************************************
// *****************************************************************************

static inline void DRV_MEMORY_UPDATE_TOKEN(uint16_t token)
{
    token++;
    token = (token == DRV_MEMORY_TOKEN_MAX) ? 1: token;
}

static void DRV_MEMORY_CallbackHandler( uintptr_t context )
{
    return;
}

/* This functions generates the Free pool of buffer objects */
static void DRV_MEMORY_BufferObjectsInit( DRV_MEMORY_OBJECT *dObj )
{
    uint32_t i;

    dObj->buffObjFree = dObj->buffObjArr;
    dObj->buffObjFree[0].index = 0;

    if(dObj->queueSize == 1)
    {
        return;
    }

    for(i=1; i < dObj->queueSize; i++ )
    {
        dObj->buffObjFree[i-1].next = &dObj->buffObjFree[i];
        dObj->buffObjFree[i].index = i;
    }
}

/* This function finds a free buffer object and populates it with the transfer
 * parameters. It also generates a new command handle for the request and
 * adds it to the queue head for processing.
 */
void DRV_MEMORY_AllocateBufferObject
(
    DRV_MEMORY_CLIENT_OBJECT *clientObj,
    DRV_MEMORY_COMMAND_HANDLE *handle,
    void *buffer,
    uint32_t blockStart,
    uint32_t nBlocks,
    DRV_MEMORY_OPERATION_TYPE opType
)
{
    DRV_MEMORY_OBJECT *dObj = &gDrvMemoryObj[clientObj->drvIndex];
    DRV_MEMORY_BUFFER_OBJECT *bufferObj = NULL;

    if(dObj->buffObjFree == (DRV_MEMORY_BUFFER_OBJECT *)NULL)
    {
        return;
    }

    /* Get transfer object from the free list */
    bufferObj = dObj->buffObjFree;
    dObj->buffObjFree = dObj->buffObjFree->next;

    bufferObj->commandHandle = DRV_MEMORY_MAKE_HANDLE(dObj->bufferToken, clientObj->drvIndex, bufferObj->index);
    bufferObj->hClient       = clientObj;
    bufferObj->buffer        = buffer;
    bufferObj->blockStart    = blockStart;
    bufferObj->nBlocks       = nBlocks;
    bufferObj->opType        = opType;
    bufferObj->status        = DRV_MEMORY_COMMAND_QUEUED;
    bufferObj->next          = (DRV_MEMORY_BUFFER_OBJECT *)NULL;

    /* Update the token number. */
    DRV_MEMORY_UPDATE_TOKEN(dObj->bufferToken);

    if (handle != NULL)
    {
        *handle = bufferObj->commandHandle;
    }

    if (dObj->queueHead == NULL)
    {
        /* This is the first buffer in the queue */
        dObj->queueHead = bufferObj;
        dObj->queueTail = bufferObj;
    }
    else
    {
        /* This means the write queue is not empty. We must add
         * the buffer object to the end of the queue */
        dObj->queueTail->next = bufferObj;
        dObj->queueTail = bufferObj;
    }
}

/* This function validates the driver handle and returns the client object
 * pointer associated with the driver handle if the handle is valid. If the
 * driver handle is not valid or if the driver is in a not ready state then
 * NULL is returned. */
static DRV_MEMORY_CLIENT_OBJECT * DRV_MEMORY_DriverHandleValidate
(
    DRV_HANDLE handle
)
{
    uint8_t instance = 0;
    uint8_t clientIndex = 0;
    DRV_MEMORY_CLIENT_OBJECT *clientObj = NULL;
    DRV_MEMORY_OBJECT *dObj = NULL;

    /* Validate the handle */
    if ((handle != DRV_HANDLE_INVALID) && (handle != 0))
    {
        instance = ((handle & DRV_MEMORY_INSTANCE_INDEX_MASK) >> 8);
        clientIndex = (handle & DRV_MEMORY_INDEX_MASK);

        if (instance >= DRV_MEMORY_INSTANCES_NUMBER)
        {
            return (NULL);
        }

        if (clientIndex >= gDrvMemoryObj[instance].nClientsMax)
        {
            return (NULL);
        }

        /* See if the client has been opened */
        clientObj = &((DRV_MEMORY_CLIENT_OBJECT *)gDrvMemoryObj[instance].clientObjPool)[clientIndex];

        if ((clientObj->clientHandle != handle) || (clientObj->inUse == false))
        {
            return (NULL);
        }

        /* Check if the driver is ready for operation */
        dObj = &gDrvMemoryObj[clientObj->drvIndex];

        if (dObj->status != SYS_STATUS_READY)
        {
            return (NULL);
        }
    }

    return (clientObj);
}

static void DRV_MEMORY_RemoveClientBufferObjects
(
    DRV_MEMORY_CLIENT_OBJECT *clientObj,
    DRV_MEMORY_OBJECT *dObj
)
{
    DRV_MEMORY_BUFFER_OBJECT *current = NULL;
    DRV_MEMORY_BUFFER_OBJECT *previous = NULL;
    DRV_MEMORY_BUFFER_OBJECT *dirty = NULL;

    current = dObj->queueHead;

    while(current != NULL)
    {
        if(clientObj == (DRV_MEMORY_CLIENT_OBJECT *)current->hClient)
        {
            /* That means this transfer object is owned
               by this client. This transfer object should
               be removed. The following code removes
               the object from a linked list queue. */
            dirty = current;

            if(previous == NULL)
            {
                dObj->queueHead = current->next;
                previous = current;
            }
            else
            {
                previous->next = current->next;
            }

            current = current->next;

            /* return the dirty object to the free list */
            dirty->next = dObj->buffObjFree;
            dObj->buffObjFree = dirty;
        }
        else
        {
            previous = current;
            current = current->next;
        }
    }

    if(dObj->queueHead != (DRV_MEMORY_BUFFER_OBJECT *)NULL)
    {
        dObj->queueTail = (DRV_MEMORY_BUFFER_OBJECT *)NULL;
    }
    else
    {
        dObj->queueTail = previous;
    }
}

/* This function updates the driver object's geometry information for the memory
 * device. */
static bool DRV_MEMORY_UpdateGeometry( DRV_MEMORY_OBJECT *dObj )
{
    MEMORY_DEVICE_GEOMETRY  memoryDeviceGeometry = { 0 };

    if (dObj->memoryDevice->GeometryGet(&memoryDeviceGeometry) == false)
    {
        return false;
    }

    /* Read block size and number of blocks */
    dObj->mediaGeometryTable[DRV_MEMORY_GEOMETRY_TABLE_READ_ENTRY].blockSize = memoryDeviceGeometry.read_blockSize;
    dObj->mediaGeometryTable[DRV_MEMORY_GEOMETRY_TABLE_READ_ENTRY].numBlocks = memoryDeviceGeometry.read_numBlocks;

    /* Write block size and number of blocks */
    dObj->mediaGeometryTable[DRV_MEMORY_GEOMETRY_TABLE_WRITE_ENTRY].blockSize = memoryDeviceGeometry.write_blockSize;
    dObj->mediaGeometryTable[DRV_MEMORY_GEOMETRY_TABLE_WRITE_ENTRY].numBlocks = memoryDeviceGeometry.write_numBlocks;
    dObj->writeBlockSize = memoryDeviceGeometry.write_blockSize;

    /* Erase block size and number of blocks */
    dObj->mediaGeometryTable[DRV_MEMORY_GEOMETRY_TABLE_ERASE_ENTRY].blockSize = memoryDeviceGeometry.erase_blockSize;
    dObj->mediaGeometryTable[DRV_MEMORY_GEOMETRY_TABLE_ERASE_ENTRY].numBlocks = memoryDeviceGeometry.erase_numBlocks;
    dObj->eraseBlockSize = memoryDeviceGeometry.erase_blockSize;

    /* Update the Media Geometry Main Structure */
    dObj->mediaGeometryObj.mediaProperty = (SYS_MEDIA_READ_IS_BLOCKING | SYS_MEDIA_WRITE_IS_BLOCKING);

    /* Number of read, write and erase entries in the table */
    dObj->mediaGeometryObj.numReadRegions = memoryDeviceGeometry.numReadRegions;
    dObj->mediaGeometryObj.numWriteRegions = memoryDeviceGeometry.numWriteRegions;
    dObj->mediaGeometryObj.numEraseRegions = memoryDeviceGeometry.numEraseRegions;
    dObj->mediaGeometryObj.geometryTable = (SYS_MEDIA_REGION_GEOMETRY *)&dObj->mediaGeometryTable;

    dObj->blockStartAddress = memoryDeviceGeometry.blockStartAddress;

    return true;
}

/* This function is responsible for handling the read operation. */
static MEMORY_DEVICE_TRANSFER_STATUS DRV_MEMORY_HandleRead
(
    DRV_MEMORY_OBJECT *dObj,
    uint8_t *data,
    uint32_t blockStart,
    uint32_t nBlocks
)
{
    uint32_t transferStatus = MEMORY_DEVICE_TRANSFER_ERROR_UNKNOWN;

    uint32_t address = (blockStart * dObj->mediaGeometryTable[0].blockSize) + dObj->blockStartAddress;

    if (dObj->memoryDevice->Read((uint32_t *)data, nBlocks, address) == true)
    {
        transferStatus = MEMORY_DEVICE_TRANSFER_COMPLETED;
    }

    return ((MEMORY_DEVICE_TRANSFER_STATUS)transferStatus);
}

static MEMORY_DEVICE_TRANSFER_STATUS DRV_MEMORY_HandleWrite
(
    DRV_MEMORY_OBJECT *dObj,
    uint8_t *data,
    uint32_t blockStart,
    uint32_t nBlocks
)
{
    uint32_t transferStatus = MEMORY_DEVICE_TRANSFER_ERROR_UNKNOWN;

    switch (dObj->writeState)
    {
        case DRV_MEMORY_WRITE_INIT:
        default:
        {
            dObj->blockAddress = ((blockStart * dObj->writeBlockSize) + dObj->blockStartAddress);
            dObj->nBlocks = nBlocks;
            dObj->writePtr = data;

            dObj->writeState = DRV_MEMORY_WRITE_MEM;
            /* Fall through */
        }

        case DRV_MEMORY_WRITE_MEM:
        {
            if (dObj->memoryDevice->PageWrite((uint32_t *)dObj->writePtr, dObj->blockAddress) == true)
            {
                dObj->writeState = DRV_MEMORY_WRITE_MEM_STATUS;
                transferStatus = MEMORY_DEVICE_TRANSFER_BUSY;
                /* Fall through For immediate check */
            }
            else
            {
                /* Break in case of failure */
                break;
            }
        }

        case DRV_MEMORY_WRITE_MEM_STATUS:
        {
            transferStatus = dObj->memoryDevice->TransferStatusGet();

            if (transferStatus == MEMORY_DEVICE_TRANSFER_COMPLETED)
            {
                dObj->nBlocks--;

                if (dObj->nBlocks != 0)
                {
                    /* There is still data to be programmed. */
                    dObj->blockAddress += dObj->writeBlockSize;
                    dObj->writePtr += dObj->writeBlockSize;

                    dObj->writeState = DRV_MEMORY_WRITE_MEM;
                    transferStatus = MEMORY_DEVICE_TRANSFER_BUSY;
                }
            }

            break;
        }
    }

    return ((MEMORY_DEVICE_TRANSFER_STATUS)transferStatus);
}

static MEMORY_DEVICE_TRANSFER_STATUS DRV_MEMORY_HandleErase
(
    DRV_MEMORY_OBJECT *dObj,
    uint8_t *data,
    uint32_t blockStart,
    uint32_t nBlocks
)
{
    uint32_t transferStatus = MEMORY_DEVICE_TRANSFER_ERROR_UNKNOWN;

    switch (dObj->eraseState)
    {
        case DRV_MEMORY_ERASE_INIT:
        default:
        {
            dObj->blockAddress = ((blockStart * dObj->eraseBlockSize) + dObj->blockStartAddress);
            dObj->nBlocks = nBlocks;
            dObj->eraseState = DRV_MEMORY_ERASE_CMD;
            /* Fall through */
        }

        case DRV_MEMORY_ERASE_CMD:
        {
            if (dObj->memoryDevice->SectorErase(dObj->blockAddress) == true)
            {
                dObj->eraseState = DRV_MEMORY_ERASE_CMD_STATUS;
                transferStatus = MEMORY_DEVICE_TRANSFER_BUSY;
                /* Fall through For immediate check */
            }
            else
            {
                /* Break in case of failure */
                break;
            }
        }

        case DRV_MEMORY_ERASE_CMD_STATUS:
        {
            transferStatus = dObj->memoryDevice->TransferStatusGet();

            if (transferStatus == MEMORY_DEVICE_TRANSFER_COMPLETED)
            {
                dObj->nBlocks--;

                if (dObj->nBlocks != 0)
                {
                    /* There is still data to be programmed. */
                    dObj->blockAddress += dObj->eraseBlockSize;

                    dObj->eraseState = DRV_MEMORY_ERASE_CMD;
                    transferStatus = MEMORY_DEVICE_TRANSFER_BUSY;
                }
            }

            break;
        }
    }

    return ((MEMORY_DEVICE_TRANSFER_STATUS)transferStatus);
}

static MEMORY_DEVICE_TRANSFER_STATUS DRV_MEMORY_HandleEraseWrite
(
    DRV_MEMORY_OBJECT *dObj,
    uint8_t *data,
    uint32_t blockStart,
    uint32_t nBlocks
)
{
    DRV_MEMORY_BUFFER_OBJECT *bufferObj = dObj->currentBufObj;
    uint8_t pagesPerSector = (dObj->eraseBlockSize / dObj->writeBlockSize);
    uint32_t readBlockStart = 0;

    uint32_t transferStatus = MEMORY_DEVICE_TRANSFER_ERROR_UNKNOWN;

    switch (dObj->ewState)
    {
        case DRV_MEMORY_EW_INIT:
        default:
        {
            dObj->eraseState = DRV_MEMORY_ERASE_INIT;
            dObj->writeState = DRV_MEMORY_WRITE_INIT;

            /* Find the sector for the starting page */
            dObj->sectorNumber = bufferObj->blockStart / pagesPerSector;

            /* Find the number of sectors to be updated in this block. */
            dObj->blockOffsetInSector = (bufferObj->blockStart % pagesPerSector);
            dObj->nBlocksToWrite = (pagesPerSector - dObj->blockOffsetInSector);

            if (bufferObj->nBlocks < dObj->nBlocksToWrite)
            {
                dObj->nBlocksToWrite = bufferObj->nBlocks;
            }

            if (dObj->nBlocksToWrite != pagesPerSector)
            {
                dObj->writePtr = dObj->ewBuffer;
                dObj->ewState = DRV_MEMORY_EW_READ_SECTOR;
            }
            else
            {
                dObj->writePtr = bufferObj->buffer;
                dObj->ewState = DRV_MEMORY_EW_ERASE_SECTOR;

                transferStatus = MEMORY_DEVICE_TRANSFER_BUSY;
            }

            /* Fall through for read operation. */
        }

        case DRV_MEMORY_EW_READ_SECTOR:
        {
            readBlockStart = (dObj->sectorNumber * dObj->eraseBlockSize);

            transferStatus = DRV_MEMORY_HandleRead (dObj, dObj->ewBuffer, readBlockStart, dObj->eraseBlockSize);

            if (transferStatus == MEMORY_DEVICE_TRANSFER_COMPLETED)
            {
                /* Find the offset from which the data is to be overlaid. */
                dObj->blockOffsetInSector *= dObj->writeBlockSize;

                memcpy ((void *)&dObj->ewBuffer[dObj->blockOffsetInSector], (const void *)bufferObj->buffer, dObj->nBlocksToWrite * dObj->writeBlockSize);

                dObj->ewState = DRV_MEMORY_EW_ERASE_SECTOR;

                transferStatus = MEMORY_DEVICE_TRANSFER_BUSY;
            }
            break;
        }

        case DRV_MEMORY_EW_ERASE_SECTOR:
        {
            transferStatus = DRV_MEMORY_HandleErase(dObj, NULL, dObj->sectorNumber, 1);
            if (transferStatus == MEMORY_DEVICE_TRANSFER_COMPLETED)
            {
                dObj->ewState = DRV_MEMORY_EW_WRITE_SECTOR;

                transferStatus = MEMORY_DEVICE_TRANSFER_BUSY;
            }
            break;
        }

        case DRV_MEMORY_EW_WRITE_SECTOR:
        {
            transferStatus = DRV_MEMORY_HandleWrite (dObj, dObj->writePtr, dObj->sectorNumber * pagesPerSector, pagesPerSector);

            if (transferStatus == MEMORY_DEVICE_TRANSFER_COMPLETED)
            {
                if ((bufferObj->nBlocks - dObj->nBlocksToWrite) == 0)
                {
                    /* This is the last write operation. */
                    break;
                }

                /* Update the number of block still to be written, sector address
                 * and the buffer pointer */
                bufferObj->nBlocks -= dObj->nBlocksToWrite;
                bufferObj->blockStart += dObj->nBlocksToWrite;
                bufferObj->buffer += (dObj->nBlocksToWrite * dObj->writeBlockSize);
                dObj->ewState = DRV_MEMORY_EW_INIT;

                transferStatus = MEMORY_DEVICE_TRANSFER_BUSY;
            }

            break;
        }
    }

    return ((MEMORY_DEVICE_TRANSFER_STATUS)transferStatus);
}

static void DRV_MEMORY_SetupXfer
(
    const DRV_HANDLE handle,
    DRV_MEMORY_COMMAND_HANDLE *commandHandle,
    void *buffer,
    uint32_t blockStart,
    uint32_t nBlock,
    uint8_t  geometry_type,
    DRV_MEMORY_OPERATION_TYPE opType,
    DRV_IO_INTENT io_intent
)
{
    DRV_MEMORY_CLIENT_OBJECT *clientObj = NULL;
    DRV_MEMORY_OBJECT *dObj = NULL;

    if (commandHandle != NULL)
    {
        *commandHandle = DRV_MEMORY_COMMAND_HANDLE_INVALID;
    }

    /* Validate the driver handle */
    clientObj = DRV_MEMORY_DriverHandleValidate(handle);

    if (clientObj == NULL)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Invalid driver handle.\n");
        return;
    }

    /* Check if the driver was opened with read intent */
    if (!(clientObj->intent & io_intent))
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Opened with invalid intent.\n");
        return;
    }

    dObj = &gDrvMemoryObj[clientObj->drvIndex];

    if ((buffer == NULL) && (opType != DRV_MEMORY_OPERATION_TYPE_ERASE))
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Invalid Buffer.\n");
        return;
    }

    if ((nBlock == 0) || ((blockStart + nBlock) > dObj->mediaGeometryTable[geometry_type].numBlocks))
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Invalid Block parameters.\n");
        return;
    }

    if (OSAL_MUTEX_Lock(&dObj->transferMutex, OSAL_WAIT_FOREVER ) == OSAL_RESULT_TRUE)
    {
        DRV_MEMORY_AllocateBufferObject (clientObj, commandHandle, buffer, blockStart, nBlock, opType);

        OSAL_MUTEX_Unlock(&dObj->transferMutex);
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: MEMORY Driver System Routines
// *****************************************************************************
// *****************************************************************************

void __attribute ((weak)) DRV_MEMORY_RegisterWithSysFs
(
    const SYS_MODULE_INDEX drvIndex,
    uint8_t mediaType
)
{

}

SYS_MODULE_OBJ DRV_MEMORY_Initialize
(
    const SYS_MODULE_INDEX drvIndex,
    const SYS_MODULE_INIT *const init
)
{
    DRV_MEMORY_OBJECT *dObj = (DRV_MEMORY_OBJECT*) NULL;
    DRV_MEMORY_INIT *memoryInit = NULL;

    /* Validate the driver index */
    if (drvIndex >= DRV_MEMORY_INSTANCES_NUMBER)
    {
        return SYS_MODULE_OBJ_INVALID;
    }

    /* Check if the instance has already been initialized. */
    if (gDrvMemoryObj[drvIndex].inUse)
    {
        return SYS_MODULE_OBJ_INVALID;
    }

    /* Assign to the local pointer the init data passed */
    memoryInit = (DRV_MEMORY_INIT *)init;

    dObj = &gDrvMemoryObj[drvIndex];

    /* Indicate that this object is in use */
    dObj->inUse = true;

    /* Initialize number of clients */
    dObj->numClients = 0;

    /* Set the driver state as busy as the Flash needs to be initialized and
     * queried for the SFDP data. */
    dObj->status              = SYS_STATUS_BUSY;

    dObj->clientObjPool       = (DRV_MEMORY_CLIENT_OBJECT *)memoryInit->clientObjPool;
    dObj->nClientsMax         = memoryInit->nClientsMax;
    dObj->buffObjArr          = (DRV_MEMORY_BUFFER_OBJECT *)memoryInit->bufferObj;
    dObj->queueSize           = memoryInit->queueSize;
    dObj->buffObjFree         = (DRV_MEMORY_BUFFER_OBJECT *)NULL;
    dObj->queueHead           = (DRV_MEMORY_BUFFER_OBJECT *)NULL;
    dObj->queueTail           = (DRV_MEMORY_BUFFER_OBJECT *)NULL;
    dObj->bufferToken         = 1;
    dObj->clientToken         = 1;

    DRV_MEMORY_BufferObjectsInit(dObj);

    dObj->inInterruptMode = memoryInit->inInterruptMode;

    /* Initialize the attached memory device functions */
    dObj->memoryDevice = memoryInit->memoryDevice;

    /* Set the erase buffer */
    dObj->ewBuffer = memoryInit->ewBuffer;

    dObj->state = DRV_MEMORY_INIT_DEVICE;

    if (dObj->memoryDevice->callbackRegister)
    {
        dObj->memoryDevice->callbackRegister(DRV_MEMORY_CallbackHandler, (uintptr_t)dObj);
    }

    if (memoryInit->isFsEnabled == true)
    {
        DRV_MEMORY_RegisterWithSysFs(drvIndex, memoryInit->deviceMediaType);
    }

    if (OSAL_MUTEX_Create(&dObj->clientMutex) == OSAL_RESULT_FALSE)
    {
        /* There was insufficient memory available for the mutex to be created */
        return SYS_MODULE_OBJ_INVALID;
    }

    if (OSAL_MUTEX_Create(&dObj->transferMutex) == OSAL_RESULT_FALSE)
    {
        /* There was insufficient memory available for the mutex to be created */
        return SYS_MODULE_OBJ_INVALID;
    }

    /* Return the driver index */
    return drvIndex;
}

SYS_STATUS DRV_MEMORY_Status
(
    SYS_MODULE_OBJ object
)
{
    /* Validate the object */
    if ((object == SYS_MODULE_OBJ_INVALID) || (object >= DRV_MEMORY_INSTANCES_NUMBER))
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO,"DRV_MEMORY_Status(): Invalid parameter.\n");
        return SYS_STATUS_UNINITIALIZED;
    }

    /* Return the driver status */
    return (gDrvMemoryObj[object].status);
}

// *****************************************************************************
// *****************************************************************************
// Section: MEMORY Driver Client Routines
// *****************************************************************************
// *****************************************************************************

DRV_HANDLE DRV_MEMORY_Open
(
    const SYS_MODULE_INDEX drvIndex,
    const DRV_IO_INTENT ioIntent
)
{
    DRV_MEMORY_CLIENT_OBJECT *clientObj = NULL;
    DRV_MEMORY_OBJECT *dObj = NULL;
    uint32_t iClient;

    /* Validate the driver index */
    if (drvIndex >= DRV_MEMORY_INSTANCES_NUMBER)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_Open(): Invalid driver index.\n");
        return DRV_HANDLE_INVALID;
    }

    dObj = &gDrvMemoryObj[drvIndex];

    /* Check if the driver is ready to be opened */
    if (dObj->status != SYS_STATUS_READY)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_Open(): Driver is not ready.\n");
        return DRV_HANDLE_INVALID;
    }

    /* Acquire the instance specific mutex to protect the instance specific
     * client pool
     */
    if (OSAL_MUTEX_Lock(&dObj->clientMutex , OSAL_WAIT_FOREVER ) == OSAL_RESULT_FALSE)
    {
        return DRV_HANDLE_INVALID;
    }

    /* Check if the driver has already been opened in exclusive mode */
    if (dObj->isExclusive)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_Open(): Driver is already open in exclusive mode.\n");
        OSAL_MUTEX_Unlock( &dObj->clientMutex);
        return DRV_HANDLE_INVALID;
    }

    /* Driver has already been opened and cannot be opened exclusively */
    if ((dObj->numClients > 0) && (ioIntent & DRV_IO_INTENT_EXCLUSIVE))
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_Open(): Driver is already open. Can't be opened in exclusive mode.\n");
        OSAL_MUTEX_Unlock( &dObj->clientMutex);
        return DRV_HANDLE_INVALID;
    }

    /* Allocate a client object from client pool */
    for(iClient = 0; iClient != dObj->nClientsMax; iClient++)
    {
        if (dObj->clientObjPool[iClient].inUse == false)
        {
            clientObj = &dObj->clientObjPool[iClient];

            /* Found a client object that can be used */
            clientObj->inUse = true;
            clientObj->drvIndex = drvIndex;
            clientObj->intent = ioIntent;
            clientObj->transferHandler = NULL;

            if (ioIntent & DRV_IO_INTENT_EXCLUSIVE)
            {
                /* Driver was opened in exclusive mode */
                dObj->isExclusive = true;
            }

            dObj->numClients ++;

            clientObj->clientHandle = DRV_MEMORY_MAKE_HANDLE(dObj->clientToken, drvIndex, iClient);

            DRV_MEMORY_UPDATE_TOKEN(dObj->clientToken);

            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_Open(): Open successful.\n");

            break;
        }
    }

    OSAL_MUTEX_Unlock(&dObj->clientMutex);

    return clientObj ? ((DRV_HANDLE)clientObj->clientHandle) : DRV_HANDLE_INVALID;
}

void DRV_MEMORY_Close
(
    const DRV_HANDLE handle
)
{
    DRV_MEMORY_CLIENT_OBJECT *clientObj = NULL;
    DRV_MEMORY_OBJECT *dObj = NULL;

    /* Get the Client object from the handle passed */
    clientObj = DRV_MEMORY_DriverHandleValidate(handle);

    /* Check if the client object is valid */
    if (clientObj == NULL)
    {
        SYS_DEBUG_PRINT (SYS_ERROR_INFO, "DRV_MEMORY_Close(): Invalid handle.\n");
        return;
    }

    dObj = &gDrvMemoryObj[clientObj->drvIndex];

    if (OSAL_MUTEX_Lock(&dObj->clientMutex , OSAL_WAIT_FOREVER ) == OSAL_RESULT_TRUE)
    {
        DRV_MEMORY_RemoveClientBufferObjects (clientObj, dObj);

        /* Update the client count */
        dObj->numClients --;
        dObj->isExclusive = false;

        /* Free the Client Instance */
        clientObj->inUse = false;

        /* Release the instance specific mutex */
        OSAL_MUTEX_Unlock( &dObj->clientMutex );

        SYS_DEBUG_PRINT (SYS_ERROR_INFO, "DRV_MEMORY_Close(): Close successful.\n");
    }
}

void DRV_MEMORY_AsyncRead
(
    const DRV_HANDLE handle,
    DRV_MEMORY_COMMAND_HANDLE *commandHandle,
    void *targetBuffer,
    uint32_t blockStart,
    uint32_t nBlock
)
{
    DRV_MEMORY_SetupXfer(handle, commandHandle, targetBuffer, blockStart, nBlock,
            DRV_MEMORY_GEOMETRY_TABLE_READ_ENTRY,
            DRV_MEMORY_OPERATION_TYPE_READ,
            DRV_IO_INTENT_READ);
}

void DRV_MEMORY_AsyncWrite
(
    const DRV_HANDLE handle,
    DRV_MEMORY_COMMAND_HANDLE *commandHandle,
    void *sourceBuffer,
    uint32_t blockStart,
    uint32_t nBlock
)
{
    DRV_MEMORY_SetupXfer(handle, commandHandle, sourceBuffer, blockStart, nBlock,
            DRV_MEMORY_GEOMETRY_TABLE_WRITE_ENTRY,
            DRV_MEMORY_OPERATION_TYPE_WRITE,
            DRV_IO_INTENT_WRITE);
}

void DRV_MEMORY_AsyncErase
(
    const DRV_HANDLE handle,
    DRV_MEMORY_COMMAND_HANDLE *commandHandle,
    uint32_t blockStart,
    uint32_t nBlock
)
{
    DRV_MEMORY_SetupXfer(handle, commandHandle, NULL, blockStart, nBlock,
            DRV_MEMORY_GEOMETRY_TABLE_ERASE_ENTRY,
            DRV_MEMORY_OPERATION_TYPE_ERASE,
            DRV_IO_INTENT_WRITE);
}

void DRV_MEMORY_AsyncEraseWrite
(
    const DRV_HANDLE handle,
    DRV_MEMORY_COMMAND_HANDLE *commandHandle,
    void *sourceBuffer,
    uint32_t blockStart,
    uint32_t nBlock
)
{
    DRV_MEMORY_SetupXfer(handle, commandHandle, sourceBuffer, blockStart, nBlock,
            DRV_MEMORY_GEOMETRY_TABLE_WRITE_ENTRY,
            DRV_MEMORY_OPERATION_TYPE_ERASE_WRITE,
            DRV_IO_INTENT_WRITE);
}

MEMORY_DEVICE_TRANSFER_STATUS DRV_MEMORY_TransferStatusGet
(
    const DRV_HANDLE handle
)
{
    DRV_MEMORY_CLIENT_OBJECT *clientObj = NULL;
    DRV_MEMORY_OBJECT *dObj = NULL;

    /* Get the Client object from the handle passed */
    clientObj = DRV_MEMORY_DriverHandleValidate(handle);

    /* Check if the client object is valid */
    if (clientObj == NULL)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_TransferStatusGet(): Invalid driver handle.\n");
        return MEMORY_DEVICE_TRANSFER_ERROR_UNKNOWN;
    }

    dObj = &gDrvMemoryObj[clientObj->drvIndex];

    return ((MEMORY_DEVICE_TRANSFER_STATUS)dObj->memoryDevice->TransferStatusGet());
}

DRV_MEMORY_COMMAND_STATUS DRV_MEMORY_CommandStatusGet
(
    const DRV_HANDLE handle,
    const DRV_MEMORY_COMMAND_HANDLE commandHandle
)
{
    uint16_t iEntry;
    DRV_MEMORY_CLIENT_OBJECT *clientObj = NULL;
    DRV_MEMORY_OBJECT *dObj = NULL;
    DRV_MEMORY_COMMAND_STATUS status = DRV_MEMORY_COMMAND_ERROR_UNKNOWN;

    /* Get the Client object from the handle passed */
    clientObj = DRV_MEMORY_DriverHandleValidate(handle);

    /* Check if the client object is valid */
    if (clientObj == NULL)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_CommandStatus(): Invalid driver handle.\n");
        return status;
    }

    dObj = &gDrvMemoryObj[clientObj->drvIndex];

    /* The lower 8 bits of the command handle is the buffer index and the
     * upper 24 bits of the command handle are the token and driver index.
     */
    iEntry = commandHandle & DRV_MEMORY_INDEX_MASK;

    /* Acquire the instance specific mutex to protect the instance specific
     * client pool
     */
    if (OSAL_MUTEX_Lock(&dObj->transferMutex , OSAL_WAIT_FOREVER ) == OSAL_RESULT_TRUE)
    {
        /* Compare the buffer handle with buffer handle in the object */
        if(dObj->buffObjArr[iEntry].commandHandle == commandHandle)
        {
            /* Return the last known buffer object status */
            status = (dObj->buffObjArr[iEntry].status);
        }
        OSAL_MUTEX_Unlock(&dObj->transferMutex);
    }

    return status;
}

void DRV_MEMORY_Tasks( SYS_MODULE_OBJ object )
{
    DRV_MEMORY_OBJECT *dObj = NULL;
    DRV_MEMORY_CLIENT_OBJECT *clientObj = NULL;
    DRV_MEMORY_BUFFER_OBJECT *bufferObj = NULL;
    DRV_MEMORY_EVENT event = DRV_MEMORY_EVENT_COMMAND_ERROR;
    bool isDone = false;
    MEMORY_DEVICE_TRANSFER_STATUS transferStatus = MEMORY_DEVICE_TRANSFER_ERROR_UNKNOWN;

    if(object == SYS_MODULE_OBJ_INVALID)
    {
        /* Invalid system object */
        return;
    }

    dObj = &gDrvMemoryObj[object];

    if (OSAL_MUTEX_Lock(&dObj->transferMutex , OSAL_WAIT_FOREVER) != OSAL_RESULT_TRUE)
    {
        return;
    }

    switch (dObj->state)
    {
        case DRV_MEMORY_INIT_DEVICE:
        {
            dObj->state = DRV_MEMORY_GEOMETRY_UPDATE;
            /* Fall Through */
        }

        case DRV_MEMORY_GEOMETRY_UPDATE:
        {
            if (true == DRV_MEMORY_UpdateGeometry(dObj))
            {
                dObj->state = DRV_MEMORY_PROCESS_QUEUE;
                dObj->status = SYS_STATUS_READY;

            }

            break;
        }

        case DRV_MEMORY_PROCESS_QUEUE:
        {
            /* Process the queued requests. */
            dObj->currentBufObj = dObj->queueHead;

            if (dObj->currentBufObj == NULL)
            {
                /* Queue is empty. Continue to remain in the same state. */
                dObj->queueTail = NULL;
                break;
            }
            else
            {
                /* Init the various sub state machines. */
                dObj->eraseState = DRV_MEMORY_ERASE_INIT;
                dObj->writeState = DRV_MEMORY_WRITE_INIT;
                dObj->ewState = DRV_MEMORY_EW_INIT;

                dObj->state = DRV_MEMORY_TRANSFER;

                dObj->currentBufObj->status = DRV_MEMORY_COMMAND_IN_PROGRESS;
            }
        }

        case DRV_MEMORY_TRANSFER:
        {
            bufferObj = dObj->currentBufObj;

            transferStatus = gMemoryXferFuncPtr[bufferObj->opType](dObj, &bufferObj->buffer[0], bufferObj->blockStart, bufferObj->nBlocks);

            if (transferStatus == MEMORY_DEVICE_TRANSFER_COMPLETED)
            {
                bufferObj->status = DRV_MEMORY_COMMAND_COMPLETED;
                event = DRV_MEMORY_EVENT_COMMAND_COMPLETE;
                isDone = true;
            }
            else if (transferStatus >= MEMORY_DEVICE_TRANSFER_ERROR_UNKNOWN)
            {
                /* The operation has failed. */
                bufferObj->status = DRV_MEMORY_COMMAND_ERROR_UNKNOWN;
                event = DRV_MEMORY_EVENT_COMMAND_ERROR;
                isDone = true;
            }

            if (isDone)
            {
                clientObj = (DRV_MEMORY_CLIENT_OBJECT *)bufferObj->hClient;

                if(clientObj->transferHandler != NULL)
                {
                    /* Call the event handler */
                    clientObj->transferHandler(event, bufferObj->commandHandle, clientObj->context);
                }

                /* Go back waiting for the next request */
                dObj->state = DRV_MEMORY_PROCESS_QUEUE;

                /* Get the next buffer in the queue */
                dObj->queueHead = dObj->queueHead->next;

                /* Return the processed buffer to free list */
                bufferObj->next = dObj->buffObjFree;
                dObj->buffObjFree = bufferObj;
            }
            break;
        }

        case DRV_MEMORY_IDLE:
        {
            break;
        }

        case DRV_MEMORY_ERROR:
        default:
        {
            break;
        }
    }

    OSAL_MUTEX_Unlock(&dObj->transferMutex);
}

void DRV_MEMORY_TransferHandlerSet
(
    const DRV_HANDLE handle,
    const void * transferHandler,
    const uintptr_t context
)
{
    DRV_MEMORY_CLIENT_OBJECT *clientObj = NULL;

    clientObj = DRV_MEMORY_DriverHandleValidate(handle);

    /* Check if the client object is valid */
    if (clientObj == NULL)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_TransferHandlerSet(): Invalid driver handle.\n");
        return;
    }

    /* Set the event handler */
    clientObj->transferHandler = transferHandler;
    clientObj->context = context;
}

SYS_MEDIA_GEOMETRY * DRV_MEMORY_GeometryGet
(
    const DRV_HANDLE handle
)
{
    DRV_MEMORY_CLIENT_OBJECT *clientObj = NULL;
    DRV_MEMORY_OBJECT *dObj = NULL;

    /* Get the Client object from the handle passed */
    clientObj = DRV_MEMORY_DriverHandleValidate(handle);

    /* Check if the client object is valid */
    if (clientObj == NULL)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_GeometryGet(): Invalid driver handle.\n");
        return NULL;
    }

    dObj = &gDrvMemoryObj[clientObj->drvIndex];
    return &dObj->mediaGeometryObj;
}

bool DRV_MEMORY_IsAttached
(
    const DRV_HANDLE handle
)
{
    /* Validate the driver handle */
    if (DRV_MEMORY_DriverHandleValidate(handle) == NULL)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_IsAttached(): Invalid driver handle.\n");
        return false;
    }

   return true;
}

bool DRV_MEMORY_IsWriteProtected
(
    const DRV_HANDLE handle
)
{
    /* This function always returns false */
    return false;
}

uintptr_t DRV_MEMORY_AddressGet
(
    const DRV_HANDLE handle
)
{
    DRV_MEMORY_CLIENT_OBJECT *clientObj = NULL;
    DRV_MEMORY_OBJECT *dObj = NULL;

    /* Get the Client object from the handle passed */
    clientObj = DRV_MEMORY_DriverHandleValidate(handle);

    /* Check if the client object is valid */
    if (clientObj == NULL)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DRV_MEMORY_AddressGet(): Invalid driver handle.\n");
        return (uintptr_t)NULL;
    }

    dObj = &gDrvMemoryObj[clientObj->drvIndex];

    return dObj->blockStartAddress;
}

