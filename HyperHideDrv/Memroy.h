#pragma once
#include "Header.h"

	enum  TzmMode
	{
		Normal,
		Call,
		Mov,
		Lea
	};

	 BOOLEAN WriteKernelMemory
	(
		PVOID pDestination,
		PVOID pSourceAddress,
		SIZE_T SizeOfCopy);
	 BOOLEAN ReadKernelMemory
	(
		PVOID pDestination,
		PVOID pSourceAddress,
		SIZE_T SizeOfCopy);


	 KIRQL WPOFFx64();
	 void  WPONx64(KIRQL irql);
	/************************************************************************
	*   Name : FindMemory
	* Param  : SearAddress ��ʼ��ַ
	* Param  : SearLenth   ��������
	* Param  : Mode        ����ģʽ CMemroyNormal//CMemroyCall//CMemroyMov
	* Param  : Tzm[5]      ������   TZM a[5] = { {0, 0}, { 0,0}, { 0,0 },  {0,0} , { 0,0} };/(������,ƫ��) e8ƫ��=0
	*     Ret: PVOID
	*  �ڴ�����
	************************************************************************/
	 PVOID FindMemory(
		PVOID   SearAddress,
		ULONG   SearLenth,
		TzmMode Mode,
		TZM    Tzm[5]);

	 PVOID FindMemoryFromReadAndWriteSection(
		 PVOID ModuleBass,
		 TzmMode Mode, 
		 TZM Tzm[5]);


	


