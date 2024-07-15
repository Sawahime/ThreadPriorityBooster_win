#include "PriorityBooster.h"

extern "C"

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	KdPrint(("PriorityBooster driver initialized successfully\n"));
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = m_PriorityBoosterUnload;

	/*ÿ������������Ҫ֧�� IRP_MJ_CREATE �� IRP_MJ_CREATE �������޷����豸�ľ��*/
	DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)m_PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)m_PriorityBoosterCreateClose;

	/*����Ϣ���ݸ���������˵�����ĸ��߳�����Ϊ�������ȼ�*/
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = m_PriorityBoosterDeviceControl;

	/*�����豸����*/
	//�����ڲ��豸��  Device �� Windows �ں��е�һ�������ռ䣬���ڹ����豸����
	//�豸���������� \Device �����ռ��´���һ����Ϊ PriorityBooster ���豸����
	//�û��ռ�������ͨ������豸����������������ͨ��,Booster.cpp�д򿪵��豸������PriorityBooster
	UNICODE_STRING m_devName = RTL_CONSTANT_STRING(L"\\Device\\PriorityBooster");

	//�������շ����豸�����ָ��
	PDEVICE_OBJECT m_DeviceObject;

	//�����豸���󲢷���״̬
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &m_devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &m_DeviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device object (0x%08X)\n", status));
		return status;
	}

	/*�����������ӣ��������豸������������*/
	/*
	\\??\\ ��һ��Ԥ����ķ�������ǰ׺�����ڽ���������ӳ�䵽�û��ռ��е�ȫ�������ռ䡣
	\\??\\ PriorityBooster ��ʾ���û��ռ��п���ʹ�õķ���������
	*/
	UNICODE_STRING m_symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");

	//������������,�û��ռ�ͨ��m_symLink���Է���m_devName
	status = IoCreateSymbolicLink(&m_symLink, &m_devName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(m_DeviceObject);
		return status;
	}

	return STATUS_SUCCESS;
}


void m_PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject) {
	//ɾ����������
	UNICODE_STRING m_symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");
	IoDeleteSymbolicLink(&m_symLink);

	//ɾ���豸����
	IoDeleteDevice(DriverObject->DeviceObject);
	KdPrint(("PriorityBooster driver unloaded successfully\n"));
}

/*Create��Close�ַ�����*/
NTSTATUS m_PriorityBoosterCreateClose(_In_ PDRIVER_OBJECT DriverObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DriverObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/*Ϊ�߳��������ȼ�*/
NTSTATUS m_PriorityBoosterDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;

	//���IO������Ŀ�����
	switch (stack->Parameters.DeviceIoControl.IoControlCode) {

		//������PriorityBoosterShared.h�Ŀ��ƴ���, Booster.cpp��DeviceIoControl�����ĵڶ�������
	case IOCTL_PRIORITY_BOOSTER_SET_PRIORITY: {
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		//�û����������뻺����(ThreadData�ṹ��)ָ��
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
		//�����߳�ID�����߳�
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &Thread);
		if (!NT_SUCCESS(status))
			break;

		//����ָ���߳�Thread�����ȼ�Ϊdata->Priority
		KeSetPriorityThread((PKTHREAD)Thread, data->Priority);
		KdPrint(("Thread Priority change for %d to %d succeeded!\n", data->ThreadId, data->Priority));

		//ÿ��ͨ��PsLookupThreadByThreadId��ȡһ������ʱ��OS�����Ӹö�������ü�������ȷ�����󲻻ᱻ�����ͷš�
		//��������Ҫʹ��Threadָ����߳�ʱ������ObDereferenceObject(Thread)�����ٶԸ��̵߳����ü���
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