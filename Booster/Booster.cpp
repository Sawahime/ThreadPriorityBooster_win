/*客户程序*/

#include "PriorityBoosterShared.h"
#include <iostream>
#include <Windows.h>

int Error(const char* message);

/*
需要从命令行传递参数给程序的情况可以使用这种形式的 main 函数
argc表示命令行参数的数量(包括程序名)
如:Booster 1234 10, Booster是程序名,它也占一个参数
argv每个元素指向一个命令行参数的字符串
*/
int main(int argc, const char* argv[]) {
	if (argc < 3) {
		std::cout << "Usage: Booster <threadid> <priority>" << std::endl;
		return 0;
	}

	//打开名为 PriorityBooster 的设备对象并返回句柄
	//\\\\.\\ 开头的路径表示，后面跟着设备的名称或符号链接. PriorityBooster.cpp已经创建了符号链接\\??\\PriorityBooster
	HANDLE m_hDevice = CreateFile(L"\\\\.\\PriorityBooster", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (m_hDevice == INVALID_HANDLE_VALUE) {
		return Error("Failed to open device\n");
	}

	ThreadData m_thread = { 0 };
	m_thread.ThreadId = atoi(argv[1]);
	m_thread.Priority = atoi(argv[2]);

	DWORD returned;
	//DeviceIoControl调用IRP_MJ_DEVICE_CONTROL到达驱动程序
	//这一部分跟PriorityVooster.cpp的m_PriorityBoosterDeviceControl函数联动
	BOOL success = DeviceIoControl(
		m_hDevice,                              // 设备句柄，已通过 CreateFile 函数打开的设备对象句柄
		IOCTL_PRIORITY_BOOSTER_SET_PRIORITY,    // 控制码，用于指定要执行的操作，通常由驱动程序定义
		&m_thread,                              // 输入缓冲区，指向要传递给驱动程序的数据结构或信息
		sizeof(m_thread),                       // 输入缓冲区大小，即输入数据结构的字节数
		nullptr,                                // 输出缓冲区，用于从驱动程序读取返回的数据（在此例中不需要）
		0,                                      // 输出缓冲区大小，因为不需要输出数据，设置为 0
		&returned,                              // 指向返回的信息的指针，通常是某种状态或返回的数据大小
		nullptr                                 // 异步 I/O 的操作句柄，这里不需要使用异步操作，设置为 nullptr
	);

	if (success) {
		std::cout << "Priority change succeeded!" << std::endl;
	}
	else {
		Error("Priority change failed!");
	}
	CloseHandle(m_hDevice);

	return 0;
}

int Error(const char* message) {
	std::cout << message << " (error=" << GetLastError() << ")" << std::endl;
	return 1;
}