/*
in this file, we have to define 2 things:
1. a struct that Driver wants from Client
2. some control program to modify the priority of thread.
*/

#pragma once

struct ThreadData {
	unsigned long ThreadId;
	int Priority;
};

/*control*/
//定义设备类型
#define PRIORITY_BOOSTER_DEVICE 0x8000
//CTL_CODE帮助创建一个特定格式的控制代码
#define IOCTL_PRIORITY_BOOSTER_SET_PRIORITY CTL_CODE(PRIORITY_BOOSTER_DEVICE, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS)
