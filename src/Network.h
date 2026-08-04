#ifndef NETWORK_H
#define NETWORK_H

#include "Globals.h"

// Task entry point for FreeRTOS
void TaskNetwork(void *pvParameters);

// Wake TaskNetwork immediately to broadcast a status update.
// Safe to call from any task context (not ISR).
inline void notifyWebUpdate()
{
    if (g_networkTaskHandle)
        xTaskNotifyGive(g_networkTaskHandle);
}

#endif
