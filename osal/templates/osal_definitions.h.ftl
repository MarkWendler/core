#ifndef __OSAL_DEFINITIONS_H
#define __OSAL_DEFINITIONS_H

<#if SELECT_RTOS == "BareMetal">
#include "osal/osal_impl_basic.h"
</#if>
<#if SELECT_RTOS == "FreeRTOS">
#include "osal/osal_freertos.h"
</#if>

#endif//__OSAL_DEFINITIONS_H
