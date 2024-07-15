#pragma once
#include "../Booster/PriorityBoosterShared.h"
#include <ntifs.h>

void m_PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS m_PriorityBoosterCreateClose(_In_ PDRIVER_OBJECT DriverObject, _In_ PIRP Irp);
NTSTATUS m_PriorityBoosterDeviceControl(PDEVICE_OBJECT, PIRP Irp);
