#include <iostream>
#include <Windows.h>
#ifdef __cplusplus
extern "C"
{
#endif
#include "DbgHelp.h"//���������Ҫ��C��ʽ�����ͷ�ļ�
#ifdef __cplusplus
}
#endif 
#pragma comment(lib , "DbgHelp.lib")
#pragma comment(lib , "ntdll.lib")
#define STATUS_UNSUCCESSFUL (0xC0000001L)
#define  SystemModuleInformation 11
#define STATUS_SUCCESS        0x00000000 
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)

extern "C" NTSTATUS __stdcall  ZwQuerySystemInformation(
    __in       ULONG SystemInformationClass,
    __inout    PVOID SystemInformation,
    __in       ULONG SystemInformationLength,
    __out_opt  PULONG ReturnLength
);


typedef BOOL(__stdcall* IMAGEUNLOAD)(
    __in  PLOADED_IMAGE LoadedImage
    );
IMAGEUNLOAD pImageUnload;
int FuncCount = 0;
typedef PLOADED_IMAGE(__stdcall* IMAGELOAD)(
    __in  PSTR DllName,
    __in  PSTR DllPath
    );
IMAGELOAD pImageLoad;


typedef BOOL(__stdcall* SYMGETSYMBOLFILE)(
    __in_opt HANDLE hProcess,
    __in_opt PCSTR SymPath,
    __in PCSTR ImageFile,
    __in DWORD Type,
    __out_ecount(cSymbolFile) PSTR SymbolFile,
    __in size_t cSymbolFile,
    __out_ecount(cDbgFile) PSTR DbgFile,
    __in size_t cDbgFile
    );
SYMGETSYMBOLFILE pSymGetSymbolFile;

typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY
{
    ULONG Unknow1;
    ULONG Unknow2;
    ULONG Unknow3;
    ULONG Unknow4;
    PVOID Base;
    ULONG Size;
    ULONG Flags;
    USHORT Index;
    USHORT NameLength;
    USHORT LoadCount;
    USHORT ModuleNameOffset;
    char ImageName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY, * PSYSTEM_MODULE_INFORMATION_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG Count;//�ں����Լ��ص�ģ��ĸ���
    SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

PLOADED_IMAGE pli;
typedef struct _tagSysModuleList {          //ģ�����ṹ
    ULONG ulCount;
    SYSTEM_MODULE_INFORMATION smi[2];
} MODULES, * PMODULES;


//-------------------------------------------------------------------------
//���������ں˺�����һ���ṹ
//-------------------------------------------------------------------------
typedef struct _KERNELFUNC_ADDRESS_INFORMATION {
    ULONG ulAddress;
    CHAR FuncName[50];
}KERNELFUNC_ADDRESS_INFORMATION, * PKERNELFUNC_ADDRESS_INFORMATION;

typedef struct _WIN32KFUNCINFO {          //PNTOSFUNCINFO
    ULONG ulCount;
    KERNELFUNC_ADDRESS_INFORMATION Win32KFuncInfo[1];
} WIN32KFUNCINFO, * PWIN32KFUNCINFO;

PWIN32KFUNCINFO FuncAddressInfo;




HANDLE hProcess;
BOOLEAN InitSymHandler()
{
    HANDLE hfile;
    char Path[MAX_PATH] = { 0 };
    char FileName[MAX_PATH] = { 0 };
    char SymPath[MAX_PATH * 2] = { 0 };
    char* SymbolsUrl = "http://msdl.microsoft.com/download/symbols";


    if (!GetCurrentDirectoryA(MAX_PATH, Path))//��ȡ��ǰĿ¼
    {
        printf("cannot get current directory \n");
        return FALSE;
    }

    strcat(Path, "\\Symbols");//����:C:\Symbols
    CreateDirectoryA(Path, NULL);//����Ŀ¼

    //���ȴ���һ��Ŀ¼ symsrv.yes�ļ���symsrv.dll���飬û�оͻᵯ��һ���Ի���Ҫ�����ȷ��

    strcpy(FileName, Path);
    strcat(FileName, "\\symsrv.yes");

    printf("%s \n", FileName);

    hfile = CreateFileA(FileName,
        FILE_ALL_ACCESS,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hfile == INVALID_HANDLE_VALUE)
    {
        printf("create or open file error: 0x%X \n", GetLastError());
        return FALSE;

    }
    CloseHandle(hfile);

    Sleep(3000);

    hProcess = GetCurrentProcess();//��ȡ��ǰ����

    //��������������
    //SYMOPT_CASE_INSENSITIVE ��ѡ��ʹ�����жԷ��������������ִ�Сд
    //

    SymSetOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);

    //��������·��
    //������win32k��pdb�����ļ����ص����Ŀ¼Path
    SymSetSearchPath(hProcess, Path);

    //����ǲ��Ǻ����죿
    //SRV*d:\localsymbols*http://msdl.microsoft.com/download/symbols
    sprintf(SymPath, "SRV*%s*%s", Path, SymbolsUrl);

    //�������ʼ��
    if (!SymInitialize(hProcess,
        SymPath,
        TRUE))
    {
        printf("SymInitialize failed %d \n", GetLastError());
        return FALSE;
    }//��ʼ������
    return TRUE;
}

ULONG GetKernelInfo(char* lpKernelName, ULONG* ulBase, ULONG* ulSize)
{
    DWORD    dwsize;
    DWORD    dwSizeReturn;
    PUCHAR    pBuffer = NULL;

    PMODULES    pSmi = NULL;
    NTSTATUS    ntStatus = STATUS_UNSUCCESSFUL;

    //�������ں˵�api����ô��������������أ�
    //ntdll!ZwQuerySystemInformation 
    //ntdll!NtQuerySystemInformation
    //���ԣ�������������õ���ntdll�ĺ�����������ntoskrnel.exe
    ntStatus = ZwQuerySystemInformation(
        SystemModuleInformation,
        pSmi,
        0,
        &dwSizeReturn
    );
    if (ntStatus != STATUS_INFO_LENGTH_MISMATCH)
    {
        return 0;
    }
    dwsize = dwSizeReturn * 2;
    pSmi = (PMODULES)new char[dwsize];
    if (pSmi == NULL)
    {
        return 0;
    }

    ntStatus = ZwQuerySystemInformation(
        SystemModuleInformation,
        pSmi,
        dwsize,
        &dwSizeReturn
    );
    if (ntStatus != STATUS_SUCCESS)
    {
        return 0;
    }
    for (int i = 0; i < pSmi->ulCount; i++)
    {
        //ѭ��������Ա�
        if (_stricmp(pSmi->smi[i].Module->ImageName, lpKernelName) == 0)
        {
            printf("found %08X %X\,,%s,,,r\n", pSmi->smi[i].Module->Base, pSmi->smi[i].Module->Size, pSmi->smi[i].Module->ImageName);
            *ulBase = (ULONG)pSmi->smi[i].Module->Base;
            *ulSize = pSmi->smi[i].Module->Size;
            break;
        }
    }
    delete pSmi;

    return TRUE;
}


BOOLEAN LoadSymModule(
    char* ImageName,
    DWORD ModuleBase)
{
    DWORD64 tmp;
    char    SymFileName[MAX_PATH] = { 0 };
    BOOL bRetOK = FALSE;

    HINSTANCE hmod = LoadLibraryA("Imagehlp.dll");
    if (!hmod)
        return FALSE;

    pImageLoad = (IMAGELOAD)GetProcAddress(hmod, "ImageLoad");
    pImageUnload = (IMAGEUNLOAD)GetProcAddress(hmod, "ImageUnload");
    if (!pImageLoad ||
        !pImageUnload)
        return FALSE;

    pli = pImageLoad(ImageName, NULL);
    if (pli == NULL)
    {
        printf("cannot get loaded module of %s \n", ImageName);
        return FALSE;
    }
    printf("ModuleName:%s:%08x\n", pli->ModuleName, pli->SizeOfImage);

    HINSTANCE hDbgHelp = LoadLibraryA("dbghelp.dll");
    if (!hDbgHelp)
        return FALSE;

    pSymGetSymbolFile = (SYMGETSYMBOLFILE)GetProcAddress(hDbgHelp, "SymGetSymbolFile");
    if (!pSymGetSymbolFile) {
        printf("pSymGetSymbolFile() failed %X\r\n", pSymGetSymbolFile);
        return FALSE;
    }
    //����������ǰ,ʹ��SymGetSymbolFile��ȡ�÷����ļ�,�������·��û������ļ�,��ô����΢�����������win32k��pdb,Ȼ�����SymLoadModule64���ؽ�������
    if (pSymGetSymbolFile(hProcess,
        NULL,
        pli->ModuleName,
        sfPdb,
        SymFileName,
        MAX_PATH,
        SymFileName,
        MAX_PATH))
    {
        //Ȼ�����SymLoadModule64��������
        tmp = SymLoadModule64(hProcess,
            pli->hFile,
            pli->ModuleName,
            NULL,
            (DWORD64)ModuleBase,
            pli->SizeOfImage);
        if (tmp)
        {
            bRetOK = TRUE;
        }
    }
    //�����ˣ���Ҫж��
    //�������ڴ棬��Ҫ�ͷ�һ���ĵ���
    //ѧjava��ͬѧҪע�⡣
    pImageUnload(pli);
    return bRetOK;
}


BOOLEAN EnumSyms(
    char* ImageName,
    DWORD ModuleBase,
    PSYM_ENUMERATESYMBOLS_CALLBACK EnumRoutine,
    PVOID Context)
{
    BOOLEAN bEnum;

    //���ȼ��ط���ģ��
    if (!LoadSymModule(ImageName, ModuleBase))
    {
        printf("cannot load symbols ,error: %d \n", GetLastError());
        return FALSE;
    }
    //��������
    bEnum = SymEnumSymbols(hProcess,
        ModuleBase,
        NULL,
        EnumRoutine, //��һ���ص�
        Context);
    if (!bEnum)
    {
        printf("cannot enum symbols ,error: %d \n", GetLastError());
    }
    return bEnum;
}


//������ǻص�����
BOOLEAN CALLBACK EnumSymRoutine(
    PSYMBOL_INFO psi,
    ULONG     SymSize,
    PVOID     Context)
{
    if (_stricmp(psi->Name, "NtUserFindWindowEx") == 0)
    {
        /*
            typedef struct _WIN32KFUNCINFO {          //PNTOSFUNCINFO
            ULONG ulCount;
            KERNELFUNC_ADDRESS_INFORMATION Win32KFuncInfo[1];
        } WIN32KFUNCINFO, *PWIN32KFUNCINFO;

        PWIN32KFUNCINFO FuncAddressInfo;
        */
        FuncAddressInfo->Win32KFuncInfo[FuncCount].ulAddress = (ULONG)psi->Address;
        strcat(FuncAddressInfo->Win32KFuncInfo[FuncCount].FuncName, psi->Name);
        FuncCount++;
    }
    if (_stricmp(psi->Name, "NtUserQueryWindow") == 0)
    {
        FuncAddressInfo->Win32KFuncInfo[FuncCount].ulAddress = (ULONG)psi->Address;
        strcat(FuncAddressInfo->Win32KFuncInfo[FuncCount].FuncName, psi->Name);
        FuncCount++;
    }
    FuncAddressInfo->ulCount = FuncCount;
    return TRUE;
}


int downloadSymbolsTest()
{
    ULONG ulBase;
    ULONG ulSize;
    //�ȳ�ʼ������
    if (InitSymHandler())
    {
        if (GetKernelInfo("\\SystemRoot\\System32\\ntoskrnl.exe", &ulBase, &ulSize))
        {

            FuncAddressInfo = (PWIN32KFUNCINFO)VirtualAlloc(0, (sizeof(WIN32KFUNCINFO) + sizeof(KERNELFUNC_ADDRESS_INFORMATION)) * 10, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (FuncAddressInfo)//����ɹ��Ļ�
            {
                memset(FuncAddressInfo, 0, (sizeof(WIN32KFUNCINFO) + sizeof(KERNELFUNC_ADDRESS_INFORMATION)) * 10);

                //�������ʼö�٣����ұ��浽�ṹ�壩
                EnumSyms("ntoskrnl.exe", ulBase, (PSYM_ENUMERATESYMBOLS_CALLBACK)EnumSymRoutine, NULL);

                //Ҫ�Ӵ�����
                SymUnloadModule64(GetCurrentProcess(), ulBase);

                //����
                SymCleanup(GetCurrentProcess());

                for (int i = 0; i < FuncAddressInfo->ulCount; i++)
                {
                    //��ӡ����
                    printf("%s[0x%08X]\r\n", FuncAddressInfo->Win32KFuncInfo[i].FuncName, FuncAddressInfo->Win32KFuncInfo[i].ulAddress);
                }
                //�����ںˣ��õ�����ѧ����ͨ���˰ɣ�
                //CallDriver(WIN32K_FUNCTION,FuncAddressInfo,(sizeof(WIN32KFUNCINFO)+sizeof(KERNELFUNC_ADDRESS_INFORMATION))*10);
            }
        }
    }
    getchar();
    return 0;
}