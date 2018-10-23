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
// <editor-fold defaultstate="collapsed" desc="DRV_I2C Instance 0 Initialization Data">

/* I2C Client Objects Pool */
DRV_I2C_CLIENT_OBJ drvI2C0ClientObjPool[DRV_I2C_CLIENTS_NUMBER_IDX0] = {0};

/* I2C Transfer Objects Pool */
DRV_I2C_TRANSFER_OBJ drvI2C0TransferObj[DRV_I2C_QUEUE_SIZE_IDX0] = {0};

/* I2C PLib Interface Initialization */
DRV_I2C_PLIB_INTERFACE drvI2C0PLibAPI = {

    /* I2C PLib Transfer Read Add function */
    .read = (DRV_I2C_READ_CALLBACK)SERCOM1_I2C_Read,

    /* I2C PLib Transfer Write Add function */
    .write = (DRV_I2C_WRITE_CALLBACK)SERCOM1_I2C_Write,

    /* I2C PLib Transfer Write Read Add function */
    .writeRead = (DRV_I2C_WRITE_READ_CALLBACK)SERCOM1_I2C_WriteRead,

    /* I2C PLib Transfer Status function */
    .errorGet = (DRV_I2C_ERROR_GET_CALLBACK)SERCOM1_I2C_ErrorGet,

    /* I2C PLib Callback Register */
    .callbackRegister = (DRV_I2C_CALLBACK_REGISTER_CALLBACK)SERCOM1_I2C_CallbackRegister,
};

/* I2C Driver Initialization Data */
DRV_I2C_INIT drvI2C0InitData =
{
    /* I2C PLib API */
    .i2cPlib = &drvI2C0PLibAPI,

    /* I2C Number of clients */
    .numClients = DRV_I2C_CLIENTS_NUMBER_IDX0,

    /* I2C Client Objects Pool */
    .clientObjPool = (uintptr_t)&drvI2C0ClientObjPool[0],

    /* I2C IRQ */
    .interruptI2C = DRV_I2C_INT_SRC_IDX0,

    /* I2C TWI Queue Size */
    .queueSize = DRV_I2C_QUEUE_SIZE_IDX0,

    /* I2C Transfer Objects */
    .transferObj = (uintptr_t)&drvI2C0TransferObj[0],

    /* I2C Clock Speed */
    .clockSpeed = DRV_I2C_CLOCK_SPEED_IDX0,
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
// <editor-fold defaultstate="collapsed" desc="SYS_TIME Initialization Data">

TIME_PLIB_API sysTimePlibAPI = {
    .timerCallbackSet = (TIME_CallbackSet)TC0_TimerCallbackRegister,
    .timerCounterGet = (TIME_CounterGet)TC0_Timer16bitCounterGet,
     .timerPeriodSet = (TIME_PeriodSet)TC0_Timer16bitPeriodSet,
    .timerFrequencyGet = (TIME_FrequencyGet)TC0_TimerFrequencyGet,
    .timerCompareSet = (TIME_CompareSet)TC0_Timer16bitCompareSet,
    .timerStart = (TIME_Start)TC0_TimerStart,
    .timerStop = (TIME_Stop)TC0_TimerStop
};

SYS_TIME_INIT sysTimeInitData =
{
    .timePlib = &sysTimePlibAPI,
    .hwTimerIntNum = TC0_IRQn,
};

// </editor-fold>


/*******************************************************************************
  Function:
    void SYS_Initialize ( void *data )

  Summary:
    Initializes the board, services, drivers, application and other modules.

  Remarks:
 */

void SYS_Initialize ( void* data )
{
    PORT_Initialize();

    CLOCK_Initialize();



    NVIC_Initialize();
	BSP_Initialize();
    EVSYS_Initialize();

    SERCOM1_I2C_Initialize();

    SERCOM4_USART_Initialize();

    TC0_TimerInitialize();


    /* Initialize I2C0 Driver Instance */
    sysObj.drvI2C0 = DRV_I2C_Initialize(DRV_I2C_INDEX_0, (SYS_MODULE_INIT *)&drvI2C0InitData);

    sysObj.sysTime = SYS_TIME_Initialize(SYS_TIME_INDEX_0, (SYS_MODULE_INIT *)&sysTimeInitData);


    APP_I2C_EEPROM_Initialize();
    APP_I2C_TEMP_SENSOR_Initialize();


}


/*******************************************************************************
 End of File
*/

