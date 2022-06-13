#include "loadDriver.h"
#include "mylog.h"
#include <Shlwapi.h>

bool installDriver(LPCWSTR serviceName, LPCWSTR displayName, LPCWSTR driverFilePath)//��װ
{
	bool bok = false;
	char chServiceName[260];
	SHTCharToAnsi(serviceName, chServiceName, 260);
	SC_HANDLE schSCManager;
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager)
	{
		SC_HANDLE schService = CreateService(schSCManager,
			serviceName,
			displayName,
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER, //�����ķ�������1Ϊ��������
			SERVICE_DEMAND_START, //���ڵ��н��̵���StartService ����ʱ�ɷ�����ƹ�����(SCM)�����ķ���
			SERVICE_ERROR_IGNORE,
			driverFilePath,//�����ļ���·��
			NULL,
			NULL,
			NULL,
			NULL,
			NULL);
		if (schService)
		{
			WriteLog(1,1,"install service:%s ok", chServiceName);
			CloseServiceHandle(schService); //������ǵ��ͷž��
			bok = true;
		}
		else
		{
			WriteLog(1,1,"install driver %s failed:%d", chServiceName, GetLastError());
		}

		CloseServiceHandle(schSCManager);
	}

	return bok;
}

bool unloadDriver(LPCWSTR serviceName)//ж��
{
	bool bok = false;
	char chServiceName[260];
	SHTCharToAnsi(serviceName, chServiceName, 260);
	SC_HANDLE schSCManager;
	SC_HANDLE hs;
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager)
	{
		hs = OpenService(schSCManager, serviceName, SERVICE_ALL_ACCESS); //�򿪷���
		if (hs)
		{
			bool a = DeleteService(hs);   //ɾ������
			if (!a)
			{
				WriteLog(1,1,"DeleteService:%s failed", chServiceName);
			}
			else
			{
				bok = true;
				WriteLog(1,1,"DeleteService:%s ok", chServiceName);
			}

			CloseServiceHandle(hs);//�ͷ����������ɴӷ��������ʧ �ͷ�ǰ��
		}
		CloseServiceHandle(schSCManager);
	}
	return bok;
}

void startDriver(LPCWSTR serviceName)//����
{
	SC_HANDLE schSCManager;
	SC_HANDLE hs;
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager)
	{
		hs = OpenService(schSCManager, serviceName, SERVICE_ALL_ACCESS); //�򿪷���
		char chserviceName[260];
		SHTCharToAnsi(serviceName, chserviceName, 260);
		if (hs)
		{
			SERVICE_STATUS serviceStatus;
			BOOL bqueryok = QueryServiceStatus(hs, &serviceStatus);
			if (bqueryok)
			{
				if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
				{
					if (StartService(hs, 0, 0))
					{
						WriteLog(1,1,"start service:%s ok", chserviceName);
					}
					else
					{
						WriteLog(1,1,"start service:%s failed:%d", chserviceName, GetLastError());
					}
				}
				else if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
				{
					WriteLog(1,1,"service:%s is running", chserviceName);
				}
			}
			else
			{
				WriteLog(1,1,"QueryServiceStatus failed:%d, start service:%s failed", GetLastError(), chserviceName);
			}

			CloseServiceHandle(hs);
		}
		else
		{
			WriteLog(1,1,"service:%s have not install yet", chserviceName);
		}
		CloseServiceHandle(schSCManager);
	}
}

void stopDriver(LPCWSTR serviceName)//ֹͣ
{
	char chServiceName[260];
	SHTCharToAnsi(serviceName, chServiceName, 260);
	SC_HANDLE schSCManager;
	SC_HANDLE hs;
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager)
	{
		hs = OpenService(schSCManager, serviceName, SERVICE_ALL_ACCESS); //�򿪷���
		if (hs)
		{
			SERVICE_STATUS status;
			int num = 0;
			if (QueryServiceStatus(hs, &status))
			{
				//if (status.dwCurrentState != SERVICE_STOPPED && status.dwCurrentState != SERVICE_STOP_PENDING)
				if (status.dwCurrentState == SERVICE_RUNNING)
				{
					ControlService(hs, SERVICE_CONTROL_STOP, &status);
					do
					{
						Sleep(50);
						num++;
						QueryServiceStatus(hs, &status);
					} while (status.dwCurrentState != SERVICE_STOPPED || num < 80);
					if (num > 80)
					{
						WriteLog(1,1,"stop service:%s failed:%d", chServiceName, GetLastError());
					}
					else
					{
						WriteLog(1,1,"stop service:%s service ok", chServiceName);
					}
				}
				else if (status.dwCurrentState == SERVICE_STOPPED)
				{
					WriteLog(1,1,"service:%s has been stoped", chServiceName);
				}
				else if (status.dwCurrentState == ERROR_SERVICE_DOES_NOT_EXIST)
				{
					WriteLog(1,1,"service:%s not exist", chServiceName);
				}
				else
				{
					WriteLog(1,1,"service:%s status:%d", chServiceName, status.dwCurrentState);
				}
			}

			CloseServiceHandle(hs);
		}
		CloseServiceHandle(schSCManager);
	}
}

