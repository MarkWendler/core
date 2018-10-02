/*******************************************************************************
  SPI Driver Definitions Header File

  Company:
    Microchip Technology Inc.

  File Name:
    drv_spi_definitions.h

  Summary:
    SPI Driver Definitions Header File

  Description:
    This file provides implementation-specific definitions for the SPI
    driver's system interface.
*******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2018 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS  WITHOUT  WARRANTY  OF  ANY  KIND,
EITHER EXPRESS  OR  IMPLIED,  INCLUDING  WITHOUT  LIMITATION,  ANY  WARRANTY  OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A  PARTICULAR  PURPOSE.
IN NO EVENT SHALL MICROCHIP OR  ITS  LICENSORS  BE  LIABLE  OR  OBLIGATED  UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION,  BREACH  OF  WARRANTY,  OR
OTHER LEGAL  EQUITABLE  THEORY  ANY  DIRECT  OR  INDIRECT  DAMAGES  OR  EXPENSES
INCLUDING BUT NOT LIMITED TO ANY  INCIDENTAL,  SPECIAL,  INDIRECT,  PUNITIVE  OR
CONSEQUENTIAL DAMAGES, LOST  PROFITS  OR  LOST  DATA,  COST  OF  PROCUREMENT  OF
SUBSTITUTE  GOODS,  TECHNOLOGY,  SERVICES,  OR  ANY  CLAIMS  BY  THIRD   PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE  THEREOF),  OR  OTHER  SIMILAR  COSTS.
*******************************************************************************/
//DOM-IGNORE-END

#ifndef DRV_SPI_DEFINITIONS_H
#define DRV_SPI_DEFINITIONS_H

// *****************************************************************************
// *****************************************************************************
// Section: File includes
// *****************************************************************************
// *****************************************************************************

#include <device.h>
#include "system/int/sys_int.h"
#include "system/dma/sys_dma.h"
#include "system/ports/sys_ports.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

    extern "C" {

#endif
// DOM-IGNORE-END

#define SYS_DEBUG(x, y)
// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

typedef enum
{
    DRV_SPI_CLOCK_PHASE_VALID_TRAILING_EDGE = 0,
    DRV_SPI_CLOCK_PHASE_VALID_LEADING_EDGE = 1,

    /* Force the compiler to reserve 32-bit memory space for each enum */
    DRV_SPI_CLOCK_PHASE_INVALID = 0xFFFFFFFF

}DRV_SPI_CLOCK_PHASE;

typedef enum
{
    DRV_SPI_CLOCK_POLARITY_IDLE_LOW = 0,
    DRV_SPI_CLOCK_POLARITY_IDLE_HIGH = 1,

    /* Force the compiler to reserve 32-bit memory space for each enum */
    DRV_SPI_CLOCK_POLARITY_INVALID = 0xFFFFFFFF

}DRV_SPI_CLOCK_POLARITY;

typedef enum
{
    DRV_SPI_DATA_BITS_8_BIT = 0,
    DRV_SPI_DATA_BITS_9_BIT = 1,
    DRV_SPI_DATA_BITS_10_BIT = 2,
    DRV_SPI_DATA_BITS_11_BIT = 3,
    DRV_SPI_DATA_BITS_12_BIT = 4,
    DRV_SPI_DATA_BITS_13_BIT = 5,
    DRV_SPI_DATA_BITS_14_BIT = 6,
    DRV_SPI_DATA_BITS_15_BIT = 7,
    DRV_SPI_DATA_BITS_16_BIT = 8,

    /* Force the compiler to reserve 32-bit memory space for each enum */
    DRV_SPI_DATA_BITS_INVALID = 0xFFFFFFFF

}DRV_SPI_DATA_BITS;

typedef enum
{
    DRV_SPI_CS_POLARITY_ACTIVE_LOW = 0,
    DRV_SPI_CS_POLARITY_ACTIVE_HIGH = 1

}DRV_SPI_CS_POLARITY;

// *****************************************************************************
/* SPI Driver Setup Data

  Summary:
    Defines the data required to setup the SPI transfer

  Description:
    This data type defines the data required to setup the SPI transfer. The
    data is passed to the DRV_SPI_TransferSetup API to setup the SPI peripheral
    settings dynamically.

  Remarks:
    None.
*/

typedef struct
{
    uint32_t                        baudRateInHz;

    DRV_SPI_CLOCK_PHASE             clockPhase;

    DRV_SPI_CLOCK_POLARITY          clockPolarity;

    DRV_SPI_DATA_BITS               dataBits;

    SYS_PORT_PIN                    chipSelect;

    DRV_SPI_CS_POLARITY             csPolarity;

} DRV_SPI_TRANSFER_SETUP;

typedef enum
{
    DRV_SPI_ERROR_NONE = 0,
    <#if .vars["${DRV_SPI_PLIB?lower_case}"].SPI_STATUS_OVERRUN_MASK?has_content>
    DRV_SPI_ERROR_OVERRUN = ${.vars["${DRV_SPI_PLIB?lower_case}"].SPI_STATUS_OVERRUN_MASK},
    </#if>

}DRV_SPI_ERROR;

typedef void (* DRV_SPI_PLIB_CALLBACK)( uintptr_t );

typedef    bool (* DRV_SETUP) (DRV_SPI_TRANSFER_SETUP *, uint32_t);

typedef    bool (* DRV_WRITEREAD)(void*, size_t, void *, size_t);

typedef    bool (* DRV_IS_BUSY)(void);

typedef    DRV_SPI_ERROR (* DRV_ERROR_GET)(void);

typedef    void (* DRV_CALLBACK_REGISTER)(DRV_SPI_PLIB_CALLBACK, uintptr_t);

// *****************************************************************************
/* SPI Driver PLIB Interface Data

  Summary:
    Defines the data required to initialize the SPI driver PLIB Interface.

  Description:
    This data type defines the data required to initialize the SPI driver
    PLIB Interface.

  Remarks:
    None.
*/

typedef struct
{
    /* SPI PLIB Setup API */
    DRV_SETUP                   setup;

    /* SPI PLIB writeRead API */
    DRV_WRITEREAD               writeRead;

    /* SPI PLIB Transfer status API */
    DRV_IS_BUSY                 isBusy;

    /* SPI PLIB Error get API */
    DRV_ERROR_GET               errorGet;

    /* SPI PLIB callback register API */
    DRV_CALLBACK_REGISTER       callbackRegister;

} DRV_SPI_PLIB_INTERFACE;

// *****************************************************************************
/* SPI Driver Initialization Data

  Summary:
    Defines the data required to initialize the SPI driver

  Description:
    This data type defines the data required to initialize or the SPI driver.

  Remarks:
    None.
*/

typedef struct
{
    /* Identifies the PLIB API set to be used by the driver to access the
     * peripheral. */
    DRV_SPI_PLIB_INTERFACE      *spiPlib;

    /* SPI transmit DMA channel. */
    SYS_DMA_CHANNEL                 dmaChannelTransmit;

    /* SPI receive DMA channel. */
    SYS_DMA_CHANNEL                 dmaChannelReceive;

    /* SPI transmit register address used for DMA operation. */
    void                        *spiTransmitAddress;

    /* SPI receive register address used for DMA operation. */
    void                        *spiReceiveAddress;

    /* Default baud rate */
    uint32_t                    baudRateInHz;

    /* Default clock phase */
    DRV_SPI_CLOCK_PHASE         clockPhase;

    /* Default clock polarity */
    DRV_SPI_CLOCK_POLARITY      clockPolarity;

    /* Default data bits */
    DRV_SPI_DATA_BITS           dataBits;

    /* Memory Pool for Client Objects */
    uintptr_t                   clientObjPool;

    /* Number of clients */
    size_t                      numClients;

    uint32_t *remapDataBits;
    uint32_t *remapClockPolarity;
    uint32_t *remapClockPhase;

<#if DRV_SPI_MODE == false>
    /* Queue for Transfer Objects */
    uintptr_t                   transferObjPool;

    /* Driver Queue Size */
    size_t                      queueSize;

    /* Interrupt source ID for SPI or DMA based on the mode used */
    INT_SOURCE                  interruptSource;
</#if>
} DRV_SPI_INIT;


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif // #ifndef DRV_SPI_DEFINITIONS_H

/*******************************************************************************
 End of File
*/

