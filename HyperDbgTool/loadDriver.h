#pragma once

#include <Windows.h>
bool installDriver(LPCWSTR serviceName, LPCWSTR displayName, LPCWSTR driverFilePath);
void startDriver(LPCWSTR serviceName);//��������
void stopDriver(LPCWSTR serviceName);//ֹͣ����
bool unloadDriver(LPCWSTR serviceName);//ж������