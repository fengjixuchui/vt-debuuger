#pragma once

#include <ntifs.h>

class CPeModule
{
public:
	/************************************************************************
*  Name : MappingPEFileInKernelSpace
*  Param: FilePath		        PE�ļ�����NT·��
*  Param: MappingBaseAddress	ӳ���Ļ���ַ ��OUT��
*  Param: MappingViewSize		�ļ�ӳ���С   ��OUT��
*  Ret  : BOOLEAN
*  ��PE�ļ�ӳ�䵽�ں˿ռ䣬ʹ�����ZwUnmapViewOfSection�ͷ�
************************************************************************/
	static BOOLEAN MappingFileToKernel
	(
		IN WCHAR* FilePath,
		OUT PVOID* MappingBaseAddress,
		OUT ULONG_PTR* MappingViewSize);

	static NTSTATUS UnMappingFileToKernel(
		_In_opt_ PVOID BaseAddress
	);
	/************************************************************************
*  Name : GetImageSection
*  Param: KernelModuleBass	ģ���ַ
*  Param: SectionName	    ������ ".data"
*  Param: SizeOfSection		������δ�С
*  Param: SectionAddress	������ε�ַ
*  Ret  : BOOLEAN
*  ��ȡָ��ģ�����ε�ַ�ʹ�С
************************************************************************/
	static BOOLEAN GetImageSection(
		IN PVOID KernelModuleBass,
		IN const char* SectionName,
		OUT PULONG SizeOfSection,
		OUT PVOID* SectionAddress
	);

	/************************************************************************
*  Name : GetSystemKernelModuleInfo
*  Param: SystemKernelModulePath	   ���ģ��·��
*  Param: SystemKernelModuleBase	   ���ģ���ַ
*  Param: SystemKernelModuleSize	   ���ģ���С
*  Ret  : BOOLEAN
*  ��ȡϵͳģ���ַ��·������С
************************************************************************/
	static BOOLEAN GetSystemKernelModuleInfo(
		OUT WCHAR** SystemKernelModulePath,
		OUT PULONG_PTR SystemKernelModuleBase,
		OUT PULONG_PTR SystemKernelModuleSize










	);
};

#define SEC_IMAGE  0x01000000