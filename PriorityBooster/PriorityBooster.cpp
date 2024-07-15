#include "PriorityBooster.h"

extern "C"

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	KdPrint(("PriorityBooster driver initialized successfully\n"));
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = m_PriorityBoosterUnload;

	/*每个驱动程序都需要支持 IRP_MJ_CREATE 和 IRP_MJ_CREATE ，否则无法打开设备的句柄*/
	DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)m_PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)m_PriorityBoosterCreateClose;

	/*将信息传递给驱动程序，说明将哪个线程设置为多少优先级*/
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = m_PriorityBoosterDeviceControl;

	/*创建设备对象*/
	//定义内部设备名  Device 是 Windows 内核中的一个命名空间，用于管理设备对象
	//设备驱动程序在 \Device 命名空间下创建一个名为 PriorityBooster 的设备对象
	//用户空间程序可以通过这个设备对象来与驱动程序通信,Booster.cpp中打开的设备名就是PriorityBooster
	UNICODE_STRING m_devName = RTL_CONSTANT_STRING(L"\\Device\\PriorityBooster");

	//用来接收返回设备对象的指针
	PDEVICE_OBJECT m_DeviceObject;

	//创建设备对象并返回状态
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &m_devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &m_DeviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device object (0x%08X)\n", status));
		return status;
	}

	/*创建符号链接，将其与设备对象连接起来*/
	/*
	\\??\\ 是一个预定义的符号链接前缀，用于将符号链接映射到用户空间中的全局命名空间。
	\\??\\ PriorityBooster 表示在用户空间中可以使用的符号链接名
	*/
	UNICODE_STRING m_symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");

	//创建符号链接,用户空间通过m_symLink可以访问m_devName
	status = IoCreateSymbolicLink(&m_symLink, &m_devName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(m_DeviceObject);
		return status;
	}

	return STATUS_SUCCESS;
}


void m_PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject) {
	//删除符号链接
	UNICODE_STRING m_symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");
	IoDeleteSymbolicLink(&m_symLink);

	//删除设备对象
	IoDeleteDevice(DriverObject->DeviceObject);
	KdPrint(("PriorityBooster driver unloaded successfully\n"));
}

/*Create和Close分发例程*/
NTSTATUS m_PriorityBoosterCreateClose(_In_ PDRIVER_OBJECT DriverObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DriverObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/*为线程设置优先级*/
NTSTATUS m_PriorityBoosterDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;

	//检查IO请求包的控制码
	switch (stack->Parameters.DeviceIoControl.IoControlCode) {

		//定义在PriorityBoosterShared.h的控制代码, Booster.cpp的DeviceIoControl函数的第二个参数
	case IOCTL_PRIORITY_BOOSTER_SET_PRIORITY: {
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		//用户传来的输入缓冲区(ThreadData结构体)指针
		auto data = (ThreadData*)stack->Parameters.DeviceIoControl.Type3InputBuffer;
		if (data == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (data->Priority < 1 || data->Priority > 31) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		PETHREAD Thread;
		//根据线程ID查找线程
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &Thread);
		if (!NT_SUCCESS(status))
			break;

		//设置指定线程Thread的优先级为data->Priority
		KeSetPriorityThread((PKTHREAD)Thread, data->Priority);
		KdPrint(("Thread Priority change for %d to %d succeeded!\n", data->ThreadId, data->Priority));

		//每次通过PsLookupThreadByThreadId获取一个对象时，OS会增加该对象的引用计数，以确保对象不会被意外释放。
		//当不再需要使用Thread指向的线程时，调用ObDereferenceObject(Thread)来减少对该线程的引用计数
		ObDereferenceObject(Thread);
		break;
	}

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}