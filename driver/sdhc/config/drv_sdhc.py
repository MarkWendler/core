# coding: utf-8
"""*****************************************************************************
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
*****************************************************************************"""

global sdhcFsEnable

def showRTOSMenu(symbol,event):
    if (event["value"] != "BareMetal"):
        # If not Bare Metal
        symbol.setVisible(True)
    else:
        symbol.setVisible(False)

def genRtosTask(symbol, event):
    symbol.setEnabled((Database.getSymbolValue("HarmonyCore", "SELECT_RTOS") != "BareMetal"))

def setVisible(symbol, event):
    symbol.setVisible(event["value"])

def setValue(symbol, event):
    symbol.setValue(int(event["value"]), 2)

def setBufferSize(symbol, event):
    if (event["id"] == "DRV_SDHC_COMMON_MODE"):
        if (event["value"] == "Asynchronous"):
            symbol.setVisible(True)
        elif(event["value"] == "Synchronous"):
            symbol.setVisible(False)
    else:
        if (event["value"] == True):
            symbol.clearValue()
            symbol.setValue(1, 1)
            symbol.setReadOnly(True)
        else:
            symbol.setReadOnly(False)

def instantiateComponent(sdhcComponent, index):
    global sdhcFsEnable

    interruptVector = "HSMCI_INTERRUPT_ENABLE"
    interruptHandler = "HSMCI_INTERRUPT_HANDLER"
    interruptHandlerLock = "HSMCI_INTERRUPT_HANDLER_LOCK"
    interruptVectorUpdate = "HSMCI_INTERRUPT_ENABLE_UPDATE"

    Database.clearSymbolValue("core", interruptVector)
    Database.setSymbolValue("core", interruptVector, True, 2)

    Database.clearSymbolValue("core", interruptHandler)
    Database.setSymbolValue("core", interruptHandler, "SDHC_InterruptHandler", 2)

    Database.clearSymbolValue("core", interruptHandlerLock)
    Database.setSymbolValue("core", interruptHandlerLock, True, 2)

    # Enable clock for HSMCI
    Database.setSymbolValue("core", "HSMCI_CLOCK_ENABLE", True, 1)

    # Enable DMA for HSMCI
    Database.setSymbolValue("core","DMA_CH_NEEDED_FOR_HSMCI", True, 2)

    # Enable dependent Harmony core components
    Database.clearSymbolValue("HarmonyCore", "ENABLE_DRV_COMMON")
    Database.setSymbolValue("HarmonyCore", "ENABLE_DRV_COMMON", True, 2)

    Database.clearSymbolValue("HarmonyCore", "ENABLE_SYS_COMMON")
    Database.setSymbolValue("HarmonyCore", "ENABLE_SYS_COMMON", True, 2)

    Database.clearSymbolValue("HarmonyCore", "ENABLE_SYS_MEDIA")
    Database.setSymbolValue("HarmonyCore", "ENABLE_SYS_MEDIA", True, 2)

    sdhcCLK = sdhcComponent.createIntegerSymbol("SDHC_CLK", None)
    sdhcCLK.setVisible(False)
    sdhcCLK.setDefaultValue(int(Database.getSymbolValue("core", "MASTER_CLOCK_FREQUENCY")))
    sdhcCLK.setDependencies(setValue, ["core.MASTER_CLOCK_FREQUENCY"])

    sdhcClients = sdhcComponent.createIntegerSymbol("DRV_SDHC_CLIENTS_NUMBER", None)
    sdhcClients.setLabel("Number of SDHC Driver Clients")
    sdhcClients.setMin(1)
    sdhcClients.setMax(10)
    sdhcClients.setDefaultValue(1)

    sdhcFsEnable = sdhcComponent.createBooleanSymbol("DRV_SDHC_FS_ENABLE", None)
    sdhcFsEnable.setLabel("File system for SDHC Driver Enabled")
    sdhcFsEnable.setDefaultValue(False)
    sdhcFsEnable.setReadOnly(True)

    sdhcBufferObjects = sdhcComponent.createIntegerSymbol("DRV_SDHC_BUFFER_QUEUE_SIZE", None)
    sdhcBufferObjects.setLabel("Buffer Queue Size")
    sdhcBufferObjects.setMin(1)
    sdhcBufferObjects.setMax(64)
    sdhcBufferObjects.setDefaultValue(1)
    sdhcBufferObjects.setVisible((Database.getSymbolValue("drv_sdhc", "DRV_SDHC_COMMON_MODE") == "Asynchronous"))
    sdhcBufferObjects.setReadOnly((sdhcFsEnable.getValue() == True))
    sdhcBufferObjects.setDependencies(setBufferSize, ["DRV_SDHC_FS_ENABLE", "drv_sdhc.DRV_SDHC_COMMON_MODE"])

    sdhcBusWidth= sdhcComponent.createComboSymbol("DRV_SDHC_TRANSFER_BUS_WIDTH", None,["1-bit", "4-bit"])
    sdhcBusWidth.setLabel("Data Transfer Bus Width")
    sdhcBusWidth.setDefaultValue("4-bit")

    sdhcBusWidth= sdhcComponent.createComboSymbol("DRV_SDHC_SDHC_BUS_SPEED", None,["DEFAULT_SPEED", "HIGH_SPEED"])
    sdhcBusWidth.setLabel("Maximum Bus Speed")
    sdhcBusWidth.setDefaultValue("DEFAULT_SPEED")

    sdhcWP = sdhcComponent.createBooleanSymbol("DRV_SDHC_SDWPEN", None)
    sdhcWP.setLabel("Use Write Protect (SDWP#) Pin")
    sdhcWP.setDefaultValue(False)

    sdhcWPComment = sdhcComponent.createCommentSymbol("DRV_SDHC_SDWPEN_COMMENT", None)
    sdhcWPComment.setLabel("*****Use pin manager to rename the Pin as SDWP*********")
    sdhcWPComment.setVisible(sdhcWP.getValue())
    sdhcWPComment.setDependencies(setVisible, ["DRV_SDHC_SDWPEN"])

    sdhcCD = sdhcComponent.createBooleanSymbol("DRV_SDHC_SDCDEN", None)
    sdhcCD.setLabel("Use Card Detect (SDCD#) Pin")
    sdhcCD.setDefaultValue(False)

    sdhcCDComment = sdhcComponent.createCommentSymbol("DRV_SDHC_SDCDEN_COMMENT", None)
    sdhcCDComment.setLabel("*****Use pin manager to rename the Pin as SDCD*********")
    sdhcCDComment.setVisible(sdhcCD.getValue())
    sdhcCDComment.setDependencies(setVisible, ["DRV_SDHC_SDCDEN"])

    sdhcDMA = sdhcComponent.createIntegerSymbol("DRV_SDHC_DMA", None)
    sdhcDMA.setLabel("DMA Channel For Transmit and Receive")
    sdhcDMA.setReadOnly(True)
    sdhcDMA.setDefaultValue(int(Database.getSymbolValue("core", "DMA_CH_FOR_HSMCI")))

    sdhcDMAChannelComment = sdhcComponent.createCommentSymbol("DRV_SDHC_DMA_CH_COMMENT", None)
    sdhcDMAChannelComment.setLabel("Warning!!! Couldn't Allocate DMA Channel. Check DMA Manager.")

    if(int(Database.getSymbolValue("core", "DMA_CH_FOR_HSMCI")) == -2):
        sdhcDMAChannelComment.setVisible(True)
    else:
        sdhcDMAChannelComment.setVisible(False)

    sdhcRTOSMenu = sdhcComponent.createMenuSymbol("SDHC_RTOS_MENU", None)
    sdhcRTOSMenu.setLabel("RTOS Configuration")
    sdhcRTOSMenu.setDescription("RTOS Configuration")
    sdhcRTOSMenu.setVisible((Database.getSymbolValue("HarmonyCore", "SELECT_RTOS") != "BareMetal"))
    sdhcRTOSMenu.setDependencies(showRTOSMenu, ["HarmonyCore.SELECT_RTOS"])

    sdhcRTOSTask = sdhcComponent.createComboSymbol("DRV_SDHC_RTOS", sdhcRTOSMenu, ["Standalone"])
    sdhcRTOSTask.setLabel("Run Library Tasks As")
    sdhcRTOSTask.setDefaultValue("Standalone")
    sdhcRTOSTask.setVisible(False)

    sdhcRTOSTaskSize = sdhcComponent.createIntegerSymbol("DRV_SDHC_RTOS_STACK_SIZE", sdhcRTOSMenu)
    sdhcRTOSTaskSize.setLabel("Stack Size")
    sdhcRTOSTaskSize.setDefaultValue(512)

    sdhcRTOSTaskPriority = sdhcComponent.createIntegerSymbol("DRV_SDHC_RTOS_TASK_PRIORITY", sdhcRTOSMenu)
    sdhcRTOSTaskPriority.setLabel("Task Priority")
    sdhcRTOSTaskPriority.setDefaultValue(1)

    sdhcRTOSTaskDelay = sdhcComponent.createBooleanSymbol("DRV_SDHC_RTOS_USE_DELAY", sdhcRTOSMenu)
    sdhcRTOSTaskDelay.setLabel("Use Task Delay?")
    sdhcRTOSTaskDelay.setDefaultValue(True)

    sdhcRTOSTaskDelayVal = sdhcComponent.createIntegerSymbol("DRV_SDHC_RTOS_DELAY", sdhcRTOSMenu)
    sdhcRTOSTaskDelayVal.setLabel("Task Delay")
    sdhcRTOSTaskDelayVal.setDefaultValue(10)
    sdhcRTOSTaskDelayVal.setVisible(sdhcRTOSTaskDelay.getValue())
    sdhcRTOSTaskDelayVal.setDependencies(setVisible, ["DRV_SDHC_RTOS_USE_DELAY"])

    configName = Variables.get("__CONFIGURATION_NAME")

    sdhcHeaderHostLocFile = sdhcComponent.createFileSymbol("DRV_SDHC_HOST_LOCAL_H", None)
    sdhcHeaderHostLocFile.setSourcePath("driver/sdhc/async/src/drv_sdhc_host_local.h")
    sdhcHeaderHostLocFile.setOutputName("drv_sdhc_host_local.h")
    sdhcHeaderHostLocFile.setDestPath("/driver/sdhc/src/")
    sdhcHeaderHostLocFile.setProjectPath("config/" + configName + "/driver/sdhc/")
    sdhcHeaderHostLocFile.setType("HEADER")

    sdhcHeaderHostFile = sdhcComponent.createFileSymbol("DRV_SDHC_HOST_H", None)
    sdhcHeaderHostFile.setSourcePath("driver/sdhc/async/src/drv_sdhc_host.h")
    sdhcHeaderHostFile.setOutputName("drv_sdhc_host.h")
    sdhcHeaderHostFile.setDestPath("/driver/sdhc/src/")
    sdhcHeaderHostFile.setProjectPath("config/" + configName + "/driver/sdhc/")
    sdhcHeaderHostFile.setType("HEADER")

    sdhcSourceHostFile = sdhcComponent.createFileSymbol("DRV_SDHC_HOST_C", None)
    sdhcSourceHostFile.setType("SOURCE")
    sdhcSourceHostFile.setOutputName("drv_sdhc_host.c")
    sdhcSourceHostFile.setSourcePath("/driver/sdhc/templates/drv_sdhc_host.c.ftl")
    sdhcSourceHostFile.setDestPath("/driver/sdhc/src/")
    sdhcSourceHostFile.setProjectPath("config/" + configName + "/driver/sdhc/")
    sdhcSourceHostFile.setMarkup(True)

    sdhcSystemInitFile = sdhcComponent.createFileSymbol("DRV_SDHC_INITIALIZE_C", None)
    sdhcSystemInitFile.setType("STRING")
    sdhcSystemInitFile.setOutputName("core.LIST_SYSTEM_INIT_C_SYS_INITIALIZE_DRIVERS")
    sdhcSystemInitFile.setSourcePath("/driver/sdhc/templates/system/system_initialize.c.ftl")
    sdhcSystemInitFile.setMarkup(True)

    sdhcSystemConfFile = sdhcComponent.createFileSymbol("DRV_SDHC_CONFIGURATION_H", None)
    sdhcSystemConfFile.setType("STRING")
    sdhcSystemConfFile.setOutputName("core.LIST_SYSTEM_CONFIG_H_DRIVER_CONFIGURATION")
    sdhcSystemConfFile.setSourcePath("/driver/sdhc/templates/system/system_config.h.ftl")
    sdhcSystemConfFile.setMarkup(True)

    sdhcSystemDataFile = sdhcComponent.createFileSymbol("DRV_SDHC_INITIALIZATION_DATA_C", None)
    sdhcSystemDataFile.setType("STRING")
    sdhcSystemDataFile.setOutputName("core.LIST_SYSTEM_INIT_C_DRIVER_INITIALIZATION_DATA")
    sdhcSystemDataFile.setSourcePath("/driver/sdhc/templates/system/system_data_initialize.c.ftl")
    sdhcSystemDataFile.setMarkup(True)

    sdhcSystemObjFile = sdhcComponent.createFileSymbol("DRV_SDHC_SYSTEM_OBJECTS_H", None)
    sdhcSystemObjFile.setType("STRING")
    sdhcSystemObjFile.setOutputName("core.LIST_SYSTEM_DEFINITIONS_H_OBJECTS")
    sdhcSystemObjFile.setSourcePath("/driver/sdhc/templates/system/system_objects.h.ftl")
    sdhcSystemObjFile.setMarkup(True)

    sdhcSystemTaskFile = sdhcComponent.createFileSymbol("DRV_SDHC_SYSTEM_TASKS_C", None)
    sdhcSystemTaskFile.setType("STRING")
    sdhcSystemTaskFile.setOutputName("core.LIST_SYSTEM_TASKS_C_CALL_SYSTEM_TASKS")
    sdhcSystemTaskFile.setSourcePath("/driver/sdhc/templates/system/system_tasks.c.ftl")
    sdhcSystemTaskFile.setMarkup(True)

    sdhcSystemRtosTasksFile = sdhcComponent.createFileSymbol("DRV_SDHC_SYS_RTOS_TASK", None)
    sdhcSystemRtosTasksFile.setType("STRING")
    sdhcSystemRtosTasksFile.setOutputName("core.LIST_SYSTEM_RTOS_TASKS_C_DEFINITIONS")
    sdhcSystemRtosTasksFile.setSourcePath("driver/sdhc/templates/system/system_rtos_tasks.c.ftl")
    sdhcSystemRtosTasksFile.setMarkup(True)
    sdhcSystemRtosTasksFile.setEnabled((Database.getSymbolValue("HarmonyCore", "SELECT_RTOS") != "BareMetal"))
    sdhcSystemRtosTasksFile.setDependencies(genRtosTask, ["HarmonyCore.SELECT_RTOS"])

def destroyComponent(sdhcComponent):
    Database.setSymbolValue("core","DMA_CH_NEEDED_FOR_HSMCI", False, 2)

def onAttachmentConnected(source, target):
    global sdhcFsEnable

    localComponent = source["component"]
    remoteComponent = target["component"]
    remoteID = remoteComponent.getID()
    connectID = source["id"]
    targetID = target["id"]

    # For Capability Connected (drv_sdcard)
    if (connectID == "drv_media"):
        if (remoteID == "sys_fs"):
            sdhcFsEnable.setValue(True, 1)
            Database.setSymbolValue("drv_sdhc", "DRV_SDHC_COMMON_FS_COUNTER", True, 1)

def onAttachmentDisconnected(source, target):
    global sdhcFsEnable

    localComponent = source["component"]
    remoteComponent = target["component"]
    remoteID = remoteComponent.getID()
    connectID = source["id"]
    targetID = target["id"]

    # For Capability Disconnected (drv_sdcard)
    if (connectID == "drv_media"):
        if (remoteID == "sys_fs"):
            sdhcFsEnable.setValue(False, 1)
            Database.setSymbolValue("drv_sdhc", "DRV_SDHC_COMMON_FS_COUNTER", False, 1)
