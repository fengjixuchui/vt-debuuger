#pragma warning( disable : 4201)
//#include <ntddk.h>
#include <intrin.h>
#include "Ntstructs.h"
#include "GlobalData.h"
#include "Log.h"
#include "Utils.h"
#include "Hider.h"
#include "Ntapi.h"
#include "HookHelper.h"
#include "Ssdt.h"
#include "HookedFunctions.h"

CONST PKUSER_SHARED_DATA KuserSharedData = (PKUSER_SHARED_DATA)KUSER_SHARED_DATA_USERMODE;

KMUTEX NtCloseMutex;

HANDLE(NTAPI* NtUserGetThreadState)(ULONG Routine);
ULONG64 KiUserExceptionDispatcherAddress = 0;

extern HYPER_HIDE_GLOBAL_DATA g_HyperHide;

NTSTATUS(NTAPI* OriginalNtQueryInformationProcess)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
NTSTATUS NTAPI HookedNtQueryInformationProcess(
	HANDLE           ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID            ProcessInformation,
	ULONG            ProcessInformationLength,
	PULONG           ReturnLength
)
{
	if (ExGetPreviousMode() == UserMode &&
		Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_QUERY_INFORMATION_PROCESS) == TRUE &&
		(ProcessInformationClass == ProcessDebugObjectHandle || ProcessInformationClass == ProcessDebugPort ||
			ProcessInformationClass == ProcessDebugFlags || ProcessInformationClass == ProcessBreakOnTermination ||
			ProcessInformationClass == ProcessBasicInformation || ProcessInformationClass == ProcessIoCounters ||
			ProcessInformationClass == ProcessHandleTracing)
		)
	{
		if (ProcessInformationLength != 0)
		{
			__try
			{
				ProbeForRead(ProcessInformation, ProcessInformationLength, 4);
				if (ReturnLength != 0)
					ProbeForWrite(ReturnLength, 4, 1);
			}

			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return GetExceptionCode();
			}
		}

		if (ProcessInformationClass == ProcessDebugObjectHandle)
		{
			if (ProcessInformationLength != sizeof(ULONG64))
				return STATUS_INFO_LENGTH_MISMATCH;

			PEPROCESS TargetProcess;
			NTSTATUS Status = ObReferenceObjectByHandle(ProcessHandle, PROCESS_QUERY_INFORMATION, *PsProcessType, UserMode, (PVOID*)&TargetProcess, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				if (Hider::IsHidden(TargetProcess, HIDE_NT_QUERY_INFORMATION_PROCESS) == TRUE)
				{
					__try
					{
						*(ULONG64*)ProcessInformation = NULL;
						if (ReturnLength != NULL) *ReturnLength = sizeof(ULONG64);

						Status = STATUS_PORT_NOT_SET;
					}

					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						Status = GetExceptionCode();
					}

					ObDereferenceObject(TargetProcess);
					return Status;
				}

				ObDereferenceObject(TargetProcess);
				return OriginalNtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
			}

			return Status;
		}

		else if (ProcessInformationClass == ProcessDebugPort)
		{
			if (ProcessInformationLength != sizeof(ULONG64))
				return STATUS_INFO_LENGTH_MISMATCH;

			PEPROCESS TargetProcess;
			NTSTATUS Status = ObReferenceObjectByHandle(ProcessHandle, PROCESS_QUERY_INFORMATION, *PsProcessType, UserMode, (PVOID*)&TargetProcess, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				if (Hider::IsHidden(TargetProcess, HIDE_NT_QUERY_INFORMATION_PROCESS) == TRUE)
				{
					__try
					{
						*(ULONG64*)ProcessInformation = 0;
						if (ReturnLength != 0)
							*ReturnLength = sizeof(ULONG64);
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						Status = GetExceptionCode();
					}

					ObDereferenceObject(TargetProcess);
					return Status;
				}

				ObDereferenceObject(TargetProcess);
				return OriginalNtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
			}

			return Status;
		}

		else if (ProcessInformationClass == ProcessDebugFlags)
		{
			if (ProcessInformationLength != 4)
				return STATUS_INFO_LENGTH_MISMATCH;

			PEPROCESS TargetProcess;
			NTSTATUS Status = ObReferenceObjectByHandle(ProcessHandle, PROCESS_QUERY_INFORMATION, *PsProcessType, UserMode, (PVOID*)&TargetProcess, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				if (Hider::IsHidden(TargetProcess, HIDE_NT_QUERY_INFORMATION_PROCESS) == TRUE)
				{
					__try
					{
						*(ULONG*)ProcessInformation = (Hider::QueryHiddenProcess(TargetProcess)->ValueProcessDebugFlags == 0) ? PROCESS_DEBUG_INHERIT : 0;
						if (ReturnLength != 0)
							*ReturnLength = sizeof(ULONG);
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						Status = GetExceptionCode();
					}

					ObDereferenceObject(TargetProcess);
					return Status;
				}

				ObDereferenceObject(TargetProcess);
				return OriginalNtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
			}

			return Status;
		}

		else if (ProcessInformationClass == ProcessBreakOnTermination)
		{
			if (ProcessInformationLength != 4)
				return STATUS_INFO_LENGTH_MISMATCH;

			PEPROCESS TargetProcess;
			NTSTATUS Status = ObReferenceObjectByHandle(ProcessHandle, 0x1000, *PsProcessType, UserMode, (PVOID*)&TargetProcess, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				if (Hider::IsHidden(TargetProcess, HIDE_NT_QUERY_INFORMATION_PROCESS) == TRUE)
				{
					__try
					{
						*(ULONG*)ProcessInformation = Hider::QueryHiddenProcess(TargetProcess)->ValueProcessBreakOnTermination;
						if (ReturnLength != 0)
							*ReturnLength = sizeof(ULONG);
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						Status = GetExceptionCode();
					}

					ObDereferenceObject(TargetProcess);
					return Status;
				}

				ObDereferenceObject(TargetProcess);
				return OriginalNtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
			}

			return Status;
		}

		NTSTATUS Status = OriginalNtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
		if (NT_SUCCESS(Status) == TRUE)
		{
			PEPROCESS TargetProcess;
			NTSTATUS ObStatus = ObReferenceObjectByHandle(ProcessHandle, 0, *PsProcessType, KernelMode, (PVOID*)&TargetProcess, NULL);

			if (NT_SUCCESS(ObStatus) == TRUE)
			{
				ObDereferenceObject(TargetProcess);

				if (Hider::IsHidden(TargetProcess, HIDE_NT_QUERY_INFORMATION_PROCESS) == TRUE)
				{
					Hider::PHIDDEN_PROCESS HiddenProcess = Hider::QueryHiddenProcess(TargetProcess);

					if (HiddenProcess != NULL)
					{
						if (ProcessInformationClass == ProcessBasicInformation)
						{
							BACKUP_RETURNLENGTH();
							PEPROCESS ExplorerProcess = GetProcessByName(L"explorer.exe");
							if (ExplorerProcess != NULL)
								((PPROCESS_BASIC_INFORMATION)ProcessInformation)->InheritedFromUniqueProcessId = (ULONG_PTR)PsGetProcessId(ExplorerProcess);
							RESTORE_RETURNLENGTH();
							return Status;
						}

						else if (ProcessInformationClass == ProcessIoCounters)
						{
							BACKUP_RETURNLENGTH();
							((PIO_COUNTERS)ProcessInformation)->OtherOperationCount = 1;
							RESTORE_RETURNLENGTH();
							return Status;
						}

						else if (ProcessInformationClass == ProcessHandleTracing)
						{
							return HiddenProcess->ProcessHandleTracingEnabled ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
						}
					}
				}
			}
		}

		return Status;
	}

	return OriginalNtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
}

NTSTATUS(NTAPI* OriginalNtSetInformationThread)(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);
NTSTATUS NTAPI HookedNtSetInformationThread(
	HANDLE ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID ThreadInformation,
	ULONG ThreadInformationLength
)
{
	PEPROCESS CurrentProcess = IoGetCurrentProcess();
	if (Hider::IsHidden(CurrentProcess, HIDE_NT_SET_INFORMATION_THREAD) == TRUE &&
		ExGetPreviousMode() == UserMode &&
		(ThreadInformationClass == ThreadHideFromDebugger || ThreadInformationClass == ThreadWow64Context ||
			ThreadInformationClass == ThreadBreakOnTermination))
	{
		if (ThreadInformationLength != 0)
		{
			__try
			{
				ProbeForRead(ThreadInformation, ThreadInformationLength, sizeof(ULONG));
			}

			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return GetExceptionCode();
			}
		}

		if (ThreadInformationClass == ThreadHideFromDebugger)
		{
			if (ThreadInformationLength != 0)
				return STATUS_INFO_LENGTH_MISMATCH;

			PETHREAD Thread;
			NTSTATUS Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_SET_INFORMATION, *PsThreadType, UserMode, (PVOID*)&Thread, NULL);

			if (NT_SUCCESS(Status) == TRUE)
			{
				PEPROCESS TargetThreadProcess = IoThreadToProcess(Thread);
				if (Hider::IsHidden(TargetThreadProcess, HIDE_NT_SET_INFORMATION_THREAD) == TRUE)
				{
					Hider::PHIDDEN_THREAD HiddenThread = Hider::AppendThreadList(TargetThreadProcess, Thread);

					HiddenThread->IsThreadHidden = TRUE;

					ObDereferenceObject(Thread);
					return Status;
				}

				ObDereferenceObject(Thread);
				return OriginalNtSetInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
			}

			return Status;
		}

		else if (ThreadInformationClass == ThreadWow64Context)
		{
			PETHREAD TargetThread;
			NTSTATUS Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_SET_CONTEXT, *PsThreadType, UserMode, (PVOID*)&TargetThread, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				PEPROCESS TargetProcess = IoThreadToProcess(TargetThread);
				if (Hider::IsHidden(TargetProcess, HIDE_NT_SET_INFORMATION_THREAD) == TRUE)
				{
					if (ThreadInformationLength != sizeof(WOW64_CONTEXT))
					{
						ObDereferenceObject(TargetThread);
						return STATUS_INFO_LENGTH_MISMATCH;
					}

					PVOID WoW64Process = PsGetCurrentProcessWow64Process();
					if (WoW64Process == 0)
					{
						ObDereferenceObject(TargetThread);
						return STATUS_INVALID_PARAMETER;
					}

					__try
					{
						PWOW64_CONTEXT Wow64Context = (PWOW64_CONTEXT)ThreadInformation;
						ULONG OriginalFlags = Wow64Context->ContextFlags;

						Wow64Context->ContextFlags &= ~0x10;

						Status = OriginalNtSetInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);

						if (OriginalFlags & 0x10)
						{
							Wow64Context->ContextFlags |= 0x10;
							Hider::PHIDDEN_THREAD HiddenThread = Hider::AppendThreadList(IoThreadToProcess(TargetThread), TargetThread);
							if (HiddenThread != NULL)
								RtlCopyBytes(&HiddenThread->FakeWow64DebugContext, &Wow64Context->Dr0, sizeof(ULONG) * 6);
						}
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						Status = GetExceptionCode();
					}

					ObDereferenceObject(TargetThread);
					return Status;
				}

				ObDereferenceObject(TargetThread);
				return OriginalNtSetInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
			}

			return Status;
		}

		else if (ThreadInformationClass == ThreadBreakOnTermination)
		{
			if (ThreadInformationLength != sizeof(ULONG))
				return STATUS_INFO_LENGTH_MISMATCH;

			__try
			{
				volatile ULONG Touch = *(ULONG*)ThreadInformation;
				UNREFERENCED_PARAMETER(Touch);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return GetExceptionCode();
			}

			LUID PrivilageValue;
			PrivilageValue.LowPart = SE_DEBUG_PRIVILEGE;
			if (SeSinglePrivilegeCheck(PrivilageValue, UserMode) == FALSE)
				return STATUS_PRIVILEGE_NOT_HELD;

			PETHREAD ThreadObject;
			NTSTATUS Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_SET_INFORMATION, *PsThreadType, ExGetPreviousMode(), (PVOID*)&ThreadObject, NULL);

			if (NT_SUCCESS(Status) == TRUE)
			{
				PEPROCESS TargetProcess = IoThreadToProcess(ThreadObject);
				if (Hider::IsHidden(TargetProcess, HIDE_NT_SET_INFORMATION_THREAD) == TRUE)
				{
					Hider::PHIDDEN_THREAD HiddenThread = Hider::AppendThreadList(TargetProcess, ThreadObject);
					if (HiddenThread != NULL)
						HiddenThread->BreakOnTermination = *(ULONG*)ThreadInformation ? TRUE : FALSE;

					ObDereferenceObject(ThreadHandle);
					return Status;
				}

				ObDereferenceObject(ThreadHandle);
				return OriginalNtSetInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
			}

			return Status;
		}
	}

	return OriginalNtSetInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
}

NTSTATUS(NTAPI* OriginalNtSetInformationProcess)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength);
NTSTATUS NTAPI HookedNtSetInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength)
{
	if (ExGetPreviousMode() == UserMode &&
		Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_SET_INFORMATION_PROCESS) == TRUE &&
		(ProcessInformationClass == ProcessBreakOnTermination || ProcessInformationClass == ProcessDebugFlags ||
			ProcessInformationClass == ProcessHandleTracing))
	{
		if (ProcessInformationLength != 0)
		{
			__try
			{
				ProbeForRead(ProcessInformation, ProcessInformationLength, 4);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return GetExceptionCode();
			}
		}

		if (ProcessInformationClass == ProcessBreakOnTermination)
		{
			if (ProcessInformationLength != sizeof(ULONG))
				return STATUS_INFO_LENGTH_MISMATCH;

			__try
			{
				volatile ULONG Touch = *(ULONG*)ProcessInformation;
				UNREFERENCED_PARAMETER(Touch);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return GetExceptionCode();
			}

			LUID PrivilageValue;
			PrivilageValue.LowPart = SE_DEBUG_PRIVILEGE;
			if (SeSinglePrivilegeCheck(PrivilageValue, UserMode) == FALSE)
				return STATUS_PRIVILEGE_NOT_HELD;

			PEPROCESS TargetProcess;
			NTSTATUS Status = ObReferenceObjectByHandle(ProcessHandle, 0x200, *PsProcessType, UserMode, (PVOID*)&TargetProcess, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				if (Hider::IsHidden(TargetProcess, HIDE_NT_SET_INFORMATION_PROCESS) == TRUE)
				{
					Hider::PHIDDEN_PROCESS HiddenProcess = Hider::QueryHiddenProcess(TargetProcess);
					if (HiddenProcess != NULL)
						HiddenProcess->ValueProcessBreakOnTermination = *(ULONG*)ProcessInformation & 1;

					ObDereferenceObject(TargetProcess);
					return Status;
				}

				ObDereferenceObject(TargetProcess);
				return OriginalNtSetInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
			}
			return Status;
		}

		else if (ProcessInformationClass == ProcessDebugFlags)
		{
			if (ProcessInformationLength != sizeof(ULONG))
				return STATUS_INFO_LENGTH_MISMATCH;

			PEPROCESS TargetProcess;
			NTSTATUS Status = ObReferenceObjectByHandle(ProcessHandle, 0x200, *PsProcessType, UserMode, (PVOID*)&TargetProcess, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				if (Hider::IsHidden(TargetProcess, HIDE_NT_SET_INFORMATION_PROCESS) == TRUE)
				{
					__try
					{
						ULONG Flags = *(ULONG*)ProcessInformation;
						if ((Flags & ~PROCESS_DEBUG_INHERIT) != 0)
						{
							ObDereferenceObject(TargetProcess);
							return STATUS_INVALID_PARAMETER;
						}

						Hider::PHIDDEN_PROCESS HiddenProcess = Hider::QueryHiddenProcess(TargetProcess);

						if ((Flags & PROCESS_DEBUG_INHERIT) != 0)
							HiddenProcess->ValueProcessDebugFlags = 0;

						else
							HiddenProcess->ValueProcessDebugFlags = TRUE;

						Status = STATUS_SUCCESS;
					}

					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						Status = GetExceptionCode();
					}

					ObDereferenceObject(TargetProcess);
					return Status;
				}

				ObDereferenceObject(TargetProcess);
				return OriginalNtSetInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
			}

			return Status;
		}

		else if (ProcessInformationClass == ProcessHandleTracing)
		{
			BOOLEAN Enable = ProcessInformationLength != 0;
			if (Enable == TRUE)
			{
				if (ProcessInformationLength != sizeof(ULONG) && ProcessInformationLength != sizeof(ULONG64))
					return STATUS_INFO_LENGTH_MISMATCH;

				__try
				{
					PPROCESS_HANDLE_TRACING_ENABLE_EX ProcessHandleTracing = (PPROCESS_HANDLE_TRACING_ENABLE_EX)ProcessInformation;
					if (ProcessHandleTracing->Flags != 0)
						return STATUS_INVALID_PARAMETER;
				}

				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					return GetExceptionCode();
				}
			}

			PEPROCESS TargetProcess;
			NTSTATUS Status = ObReferenceObjectByHandle(ProcessHandle, 0x200, *PsProcessType, UserMode, (PVOID*)&TargetProcess, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				if (Hider::IsHidden(TargetProcess, HIDE_NT_SET_INFORMATION_PROCESS) == TRUE)
				{
					Hider::PHIDDEN_PROCESS HiddenProcess = Hider::QueryHiddenProcess(TargetProcess);
					if (HiddenProcess != NULL)
						HiddenProcess->ProcessHandleTracingEnabled = Enable;

					ObDereferenceObject(TargetProcess);
					return Status;
				}

				ObDereferenceObject(TargetProcess);
				return OriginalNtSetInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
			}

			return Status;
		}
	}
	return OriginalNtSetInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
}

NTSTATUS(NTAPI* OriginalNtQueryObject)(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);
NTSTATUS NTAPI HookedNtQueryObject(
	HANDLE                   Handle,
	OBJECT_INFORMATION_CLASS ObjectInformationClass,
	PVOID                    ObjectInformation,
	ULONG                    ObjectInformationLength,
	PULONG                   ReturnLength
)
{
	NTSTATUS Status = OriginalNtQueryObject(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_QUERY_OBJECT) == TRUE &&
		NT_SUCCESS(Status) == TRUE &&
		ExGetPreviousMode() == UserMode &&
		ObjectInformation != NULL)
	{

		if (ObjectInformationClass == ObjectTypeInformation)
		{
			UNICODE_STRING DebugObject;
			RtlInitUnicodeString(&DebugObject, L"DebugObject");
			POBJECT_TYPE_INFORMATION Type = (POBJECT_TYPE_INFORMATION)ObjectInformation;

			if (RtlEqualUnicodeString(&Type->TypeName, &DebugObject, FALSE) == TRUE)
			{
				BACKUP_RETURNLENGTH();
				Type->TotalNumberOfObjects -= g_HyperHide.NumberOfActiveDebuggers;
				Type->TotalNumberOfHandles -= g_HyperHide.NumberOfActiveDebuggers;
				RESTORE_RETURNLENGTH();
			}

			return Status;
		}

		else if (ObjectInformationClass == ObjectTypesInformation)
		{
			UNICODE_STRING DebugObject;
			RtlInitUnicodeString(&DebugObject, L"DebugObject");
			POBJECT_ALL_INFORMATION ObjectAllInfo = (POBJECT_ALL_INFORMATION)ObjectInformation;
			UCHAR* ObjInfoLocation = (UCHAR*)ObjectAllInfo->ObjectTypeInformation;
			ULONG TotalObjects = ObjectAllInfo->NumberOfObjectsTypes;

			BACKUP_RETURNLENGTH();
			for (ULONG i = 0; i < TotalObjects; i++)
			{
				POBJECT_TYPE_INFORMATION ObjectTypeInfo = (POBJECT_TYPE_INFORMATION)ObjInfoLocation;
				if (RtlEqualUnicodeString(&ObjectTypeInfo->TypeName, &DebugObject, FALSE) == TRUE)
				{
					ObjectTypeInfo->TotalNumberOfObjects = 0;
					ObjectTypeInfo->TotalNumberOfHandles = 0;
				}
				ObjInfoLocation = (UCHAR*)ObjectTypeInfo->TypeName.Buffer;
				ObjInfoLocation += ObjectTypeInfo->TypeName.MaximumLength;
				ULONG64 Tmp = ((ULONG64)ObjInfoLocation) & -(LONG64)sizeof(PVOID);
				if ((ULONG64)Tmp != (ULONG64)ObjInfoLocation)
					Tmp += sizeof(PVOID);
				ObjInfoLocation = ((UCHAR*)Tmp);
			}
			RESTORE_RETURNLENGTH();
			return Status;
		}
	}

	return Status;
}

NTSTATUS(NTAPI* OriginalNtSystemDebugControl)(SYSDBG_COMMAND Command, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG ReturnLength);
NTSTATUS NTAPI HookedNtSystemDebugControl(
	SYSDBG_COMMAND       Command,
	PVOID                InputBuffer,
	ULONG                InputBufferLength,
	PVOID               OutputBuffer,
	ULONG                OutputBufferLength,
	PULONG              ReturnLength)
{
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_SYSTEM_DEBUG_CONTROL) == TRUE)
	{
		if (Command == SysDbgGetTriageDump)
			return STATUS_INFO_LENGTH_MISMATCH;
		return STATUS_DEBUGGER_INACTIVE;
	}
	return OriginalNtSystemDebugControl(Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);
}

NTSTATUS(NTAPI* OriginalNtClose)(HANDLE Handle);
NTSTATUS NTAPI HookedNtClose(HANDLE Handle)
{
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_CLOSE) == TRUE)
	{
		KeWaitForSingleObject(&NtCloseMutex, Executive, KernelMode, FALSE, NULL);

		OBJECT_HANDLE_ATTRIBUTE_INFORMATION ObjAttributeInfo;

		NTSTATUS Status = ZwQueryObject(Handle, (OBJECT_INFORMATION_CLASS)4 /*ObjectDataInformation*/, &ObjAttributeInfo, sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION), NULL);

		if (Status == STATUS_INVALID_HANDLE)
		{
			KeReleaseMutex(&NtCloseMutex, FALSE);
			return STATUS_INVALID_HANDLE;
		}

		if (NT_SUCCESS(Status) == TRUE)
		{
			if (ObjAttributeInfo.ProtectFromClose == TRUE)
			{
				KeReleaseMutex(&NtCloseMutex, FALSE);
				return STATUS_HANDLE_NOT_CLOSABLE;
			}
		}

		KeReleaseMutex(&NtCloseMutex, FALSE);
	}

	return OriginalNtClose(Handle);
}

NTSTATUS(NTAPI* OriginalNtGetNextProcess)(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, ULONG HandleAttributes, ULONG Flags, PHANDLE NewProcessHandle);
NTSTATUS NTAPI HookedNtGetNextProcess(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, ULONG HandleAttributes, ULONG Flags, PHANDLE NewProcessHandle)
{
	NTSTATUS Status = OriginalNtGetNextProcess(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_GET_NEXT_PROCESS) == TRUE &&
		ExGetPreviousMode() == UserMode &&
		NT_SUCCESS(Status) == TRUE)
	{
		PEPROCESS NewProcess;
		NTSTATUS ObStatus = ObReferenceObjectByHandle(*NewProcessHandle, NULL, *PsProcessType, KernelMode, (PVOID*)&NewProcess, NULL);
		if (NT_SUCCESS(ObStatus) == TRUE)
		{
			UNICODE_STRING ProcessImageName = PsQueryFullProcessImageName(NewProcess);
			if (Hider::IsProcessNameBad(&ProcessImageName) == TRUE)
			{
				HANDLE OldHandleValue = *NewProcessHandle;

				Status = HookedNtGetNextProcess(*NewProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
				ObCloseHandle(OldHandleValue, UserMode);
			}

			ObDereferenceObject(NewProcess);
			return Status;
		}

		return Status;
	}

	return OriginalNtGetNextProcess(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
}

NTSTATUS(NTAPI* OriginalNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
NTSTATUS NTAPI HookedNtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength)
{
	NTSTATUS Status = OriginalNtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
	PEPROCESS CurrentProcess = IoGetCurrentProcess();

	if (ExGetPreviousMode() == UserMode &&
		Hider::IsHidden(CurrentProcess, HIDE_NT_QUERY_SYSTEM_INFORMATION) == TRUE &&
		NT_SUCCESS(Status) == TRUE
		)
	{
		if (SystemInformationClass == SystemKernelDebuggerInformation)
		{
			PSYSTEM_KERNEL_DEBUGGER_INFORMATION DebuggerInfo = (PSYSTEM_KERNEL_DEBUGGER_INFORMATION)SystemInformation;

			BACKUP_RETURNLENGTH();
			DebuggerInfo->DebuggerEnabled = 0;
			DebuggerInfo->DebuggerNotPresent = 1;
			RESTORE_RETURNLENGTH();
		}

		else if (SystemInformationClass == SystemProcessInformation ||
			SystemInformationClass == SystemSessionProcessInformation ||
			SystemInformationClass == SystemExtendedProcessInformation ||
			SystemInformationClass == SystemFullProcessInformation)
		{
			PSYSTEM_PROCESS_INFO ProcessInfo = (PSYSTEM_PROCESS_INFO)SystemInformation;
			if (SystemInformationClass == SystemSessionProcessInformation)
				ProcessInfo = (PSYSTEM_PROCESS_INFO)((PSYSTEM_SESSION_PROCESS_INFORMATION)SystemInformation)->Buffer;

			BACKUP_RETURNLENGTH();
			DbgPrint("FilterProcesses\n");
			FilterProcesses(ProcessInfo);

			for (PSYSTEM_PROCESS_INFO Entry = ProcessInfo; Entry->NextEntryOffset != NULL; Entry = (PSYSTEM_PROCESS_INFO)((UCHAR*)Entry + Entry->NextEntryOffset))
			{
				if (Hider::IsHidden(PidToProcess(Entry->ProcessId), HIDE_NT_QUERY_SYSTEM_INFORMATION) == TRUE)
				{
					PEPROCESS ExplorerProcess = GetProcessByName(L"explorer.exe");
					if (ExplorerProcess != NULL)
						Entry->InheritedFromProcessId = PsGetProcessId(ExplorerProcess);

					Entry->OtherOperationCount.QuadPart = 1;
				}
			}
			RESTORE_RETURNLENGTH();
		}

		else if (SystemInformationClass == SystemCodeIntegrityInformation)
		{
			BACKUP_RETURNLENGTH();
			((PSYSTEM_CODEINTEGRITY_INFORMATION)SystemInformation)->CodeIntegrityOptions = 0x1; // CODEINTEGRITY_OPTION_ENABLED
			RESTORE_RETURNLENGTH();
		}

		else if (SystemInformationClass == SystemKernelDebuggerInformationEx)
		{
			BACKUP_RETURNLENGTH();
			((PSYSTEM_KERNEL_DEBUGGER_INFORMATION_EX)SystemInformation)->DebuggerAllowed = FALSE;
			((PSYSTEM_KERNEL_DEBUGGER_INFORMATION_EX)SystemInformation)->DebuggerEnabled = FALSE;
			((PSYSTEM_KERNEL_DEBUGGER_INFORMATION_EX)SystemInformation)->DebuggerPresent = FALSE;
			RESTORE_RETURNLENGTH();
		}

		else if (SystemInformationClass == SystemKernelDebuggerFlags)
		{
			BACKUP_RETURNLENGTH();
			*(UCHAR*)SystemInformation = NULL;
			RESTORE_RETURNLENGTH();
		}

		else if (SystemInformationClass == SystemExtendedHandleInformation)
		{
			PSYSTEM_HANDLE_INFORMATION_EX HandleInfoEx = (PSYSTEM_HANDLE_INFORMATION_EX)SystemInformation;

			BACKUP_RETURNLENGTH();
			FilterHandlesEx(HandleInfoEx);
			RESTORE_RETURNLENGTH();
		}

		else if (SystemInformationClass == SystemHandleInformation)
		{
			PSYSTEM_HANDLE_INFORMATION HandleInfo = (PSYSTEM_HANDLE_INFORMATION)SystemInformation;

			BACKUP_RETURNLENGTH();
			FilterHandles(HandleInfo);
			RESTORE_RETURNLENGTH();
		}
	}

	return Status;
}

NTSTATUS(NTAPI* OriginalNtSetContextThread)(HANDLE ThreadHandle, PCONTEXT Context);
NTSTATUS NTAPI HookedNtSetContextThread(HANDLE ThreadHandle, PCONTEXT Context)
{
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_SET_CONTEXT_THREAD) == TRUE &&
		ExGetPreviousMode() == UserMode)
	{
		PETHREAD TargethThread;
		NTSTATUS Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_SET_CONTEXT, *PsThreadType, UserMode, (PVOID*)&TargethThread, 0);
		if (NT_SUCCESS(Status) == TRUE)
		{
			PEPROCESS TargetProcess = IoThreadToProcess(TargethThread);
			if (Hider::IsHidden(TargetProcess, HIDE_NT_SET_CONTEXT_THREAD) == TRUE)
			{
				if (IsSetThreadContextRestricted(TargetProcess) == TRUE && IoThreadToProcess(PsGetCurrentThread()) == TargetProcess)
				{
					ObDereferenceObject(TargethThread);
					return STATUS_SET_CONTEXT_DENIED;
				}

				// If it is a system thread or pico process thread return STATUS_INVALID_HANDLE
				if (IoIsSystemThread(TargethThread) == TRUE || IsPicoContextNull(TargethThread) == FALSE)
				{
					ObDereferenceObject(TargethThread);
					return STATUS_INVALID_HANDLE;
				}

				__try
				{
					ULONG OriginalFlags = Context->ContextFlags;

					Context->ContextFlags &= ~0x10;

					Status = OriginalNtSetContextThread(ThreadHandle, Context);

					if (OriginalFlags & 0x10)
					{
						Context->ContextFlags |= 0x10;

						Hider::PHIDDEN_THREAD HiddenThread = Hider::AppendThreadList(TargetProcess, TargethThread);
						if (HiddenThread != 0)
						{
							RtlCopyBytes(&HiddenThread->FakeDebugContext.DR0, &Context->Dr0, sizeof(ULONG64) * 6);
							RtlCopyBytes(&HiddenThread->FakeDebugContext.DebugControl, &Context->DebugControl, sizeof(ULONG64) * 5);
						}
					}
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					Status = GetExceptionCode();
				}

				ObDereferenceObject(TargethThread);
				return Status;
			}

			ObDereferenceObject(TargethThread);
			return OriginalNtSetContextThread(ThreadHandle, Context);
		}

		return Status;
	}

	return OriginalNtSetContextThread(ThreadHandle, Context);
}

NTSTATUS(NTAPI* OriginalNtGetContextThread)(IN HANDLE ThreadHandle, IN OUT PCONTEXT Context);
NTSTATUS NTAPI HookedNtGetContextThread(IN HANDLE ThreadHandle, IN OUT PCONTEXT Context)
{
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_GET_CONTEXT_THREAD) == TRUE &&
		ExGetPreviousMode() == UserMode
		)
	{
		PETHREAD ThreadObject;
		NTSTATUS Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_SET_CONTEXT, *PsThreadType, UserMode, (PVOID*)&ThreadObject, 0);
		if (NT_SUCCESS(Status) == TRUE)
		{
			// If it is a system thread return STATUS_INVALID_HANDLE
			if (IoIsSystemThread(ThreadObject) == TRUE)
			{
				ObDereferenceObject(ThreadObject);
				return STATUS_INVALID_HANDLE;
			}

			// Check if thread object belongs to any hidden process
			if (Hider::IsHidden(IoThreadToProcess(ThreadObject), HIDE_NT_SET_CONTEXT_THREAD) == TRUE)
			{
				__try
				{
					ULONG OriginalFlags = Context->ContextFlags;

					Context->ContextFlags &= ~0x10;

					Status = OriginalNtGetContextThread(ThreadHandle, Context);

					if (OriginalFlags & 0x10)
					{
						Context->ContextFlags |= 0x10;

						Hider::PHIDDEN_THREAD HiddenThread = Hider::AppendThreadList(IoThreadToProcess(ThreadObject), ThreadObject);
						if (HiddenThread != NULL)
						{
							RtlCopyBytes(&Context->Dr0, &HiddenThread->FakeDebugContext.DR0, sizeof(ULONG64) * 6);
							RtlCopyBytes(&Context->DebugControl, &HiddenThread->FakeDebugContext.DebugControl, sizeof(ULONG64) * 5);
						}
						else
						{
							RtlSecureZeroMemory(&Context->Dr0, sizeof(ULONG64) * 6);
							RtlSecureZeroMemory(&Context->DebugControl, sizeof(ULONG64) * 5);
						}
					}
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					Status = GetExceptionCode();
				}

				ObDereferenceObject(ThreadObject);
				return Status;
			}

			ObDereferenceObject(ThreadObject);
			return OriginalNtGetContextThread(ThreadHandle, Context);
		}

		return Status;
	}

	return OriginalNtGetContextThread(ThreadHandle, Context);
}

NTSTATUS(NTAPI* OriginalNtQueryInformationThread)(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength, PULONG ReturnLength);
NTSTATUS NTAPI HookedNtQueryInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength, PULONG ReturnLength)
{
	PEPROCESS CurrentProcess = IoGetCurrentProcess();
	if (Hider::IsHidden(CurrentProcess, HIDE_NT_QUERY_INFORMATION_THREAD) == TRUE &&
		ExGetPreviousMode() == UserMode && (ThreadInformationClass == ThreadHideFromDebugger ||
			ThreadInformationClass == ThreadBreakOnTermination || ThreadInformationClass == ThreadWow64Context))
	{
		if (ThreadInformationLength != 0)
		{
			__try
			{
				ProbeForRead(ThreadInformation, ThreadInformationLength, 4);
				if (ReturnLength != 0)
					ProbeForWrite(ReturnLength, 4, 1);

			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return GetExceptionCode();
			}
		}

		if (ThreadInformationClass == ThreadHideFromDebugger)
		{
			if (ThreadInformationLength != 1)
				return STATUS_INFO_LENGTH_MISMATCH;

			PETHREAD TargetThread;
			NTSTATUS Status = ObReferenceObjectByHandle(ThreadHandle, 0x40, *PsThreadType, UserMode, (PVOID*)&TargetThread, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				if (Hider::IsHidden(IoThreadToProcess(TargetThread), HIDE_NT_QUERY_INFORMATION_THREAD) == TRUE)
				{
					__try
					{
						*(BOOLEAN*)ThreadInformation = Hider::AppendThreadList(IoThreadToProcess(TargetThread), TargetThread)->IsThreadHidden;

						if (ReturnLength != 0) *ReturnLength = 1;
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						Status = GetExceptionCode();
					}

					ObDereferenceObject(TargetThread);
					return Status;
				}

				ObDereferenceObject(TargetThread);
				return OriginalNtQueryInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
			}

			return Status;
		}

		if (ThreadInformationClass == ThreadBreakOnTermination)
		{
			if (ThreadInformationLength != 4)
				return STATUS_INFO_LENGTH_MISMATCH;

			PETHREAD TargetThread;
			NTSTATUS Status = ObReferenceObjectByHandle(ThreadHandle, 0x40, *PsThreadType, UserMode, (PVOID*)&TargetThread, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				if (Hider::IsHidden(IoThreadToProcess(TargetThread), HIDE_NT_QUERY_INFORMATION_THREAD) == TRUE)
				{
					__try
					{
						*(ULONG*)ThreadInformation = Hider::AppendThreadList(IoThreadToProcess(TargetThread), TargetThread)->BreakOnTermination;

						if (ReturnLength != NULL) *ReturnLength = 4;
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						Status = GetExceptionCode();
					}

					ObDereferenceObject(TargetThread);
					return Status;
				}

				ObDereferenceObject(TargetThread);
				return OriginalNtQueryInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
			}

			return Status;
		}

		if (ThreadInformationClass == ThreadWow64Context)
		{
			PETHREAD TargetThread;
			NTSTATUS Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_GET_CONTEXT, *PsThreadType, UserMode, (PVOID*)&TargetThread, NULL);
			if (NT_SUCCESS(Status) == TRUE)
			{
				if (Hider::IsHidden(IoThreadToProcess(TargetThread), HIDE_NT_QUERY_INFORMATION_THREAD) == TRUE)
				{
					if (ThreadInformationLength != sizeof(WOW64_CONTEXT))
					{
						ObDereferenceObject(TargetThread);
						return STATUS_INFO_LENGTH_MISMATCH;
					}

					PVOID WoW64Process = PsGetCurrentProcessWow64Process();
					if (WoW64Process == 0)
					{
						ObDereferenceObject(TargetThread);
						return STATUS_INVALID_PARAMETER;
					}

					__try
					{
						PWOW64_CONTEXT Context = (PWOW64_CONTEXT)ThreadInformation;
						ULONG OriginalFlags = Context->ContextFlags;

						Context->ContextFlags &= ~0x10;

						Status = OriginalNtQueryInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);

						if (OriginalFlags & 0x10)
						{
							Context->ContextFlags |= 0x10;

							Hider::PHIDDEN_THREAD HiddenThread = Hider::AppendThreadList(IoThreadToProcess(TargetThread), TargetThread);

							if (HiddenThread != NULL)
								RtlCopyBytes(&Context->Dr0, &HiddenThread->FakeWow64DebugContext, sizeof(ULONG) * 6);

							else
								RtlSecureZeroMemory(&Context->Dr0, sizeof(ULONG) * 6);
						}
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						Status = GetExceptionCode();
					}

					ObDereferenceObject(TargetThread);
					return Status;
				}

				ObDereferenceObject(TargetThread);
				return OriginalNtQueryInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
			}

			return Status;
		}
	}

	return OriginalNtQueryInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
}

NTSTATUS(NTAPI* OriginalNtOpenProcess)(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);
NTSTATUS NTAPI HookedNtOpenProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId)
{
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_OPEN_PROCESS) == TRUE &&
		ExGetPreviousMode() == UserMode)
	{
		__try
		{
			ProbeForWrite(ProcessHandle, 4, 1);
			ProbeForWrite(ObjectAttributes, 28, 4);
		}

		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return GetExceptionCode();
		}

		if (ClientId != NULL)
		{
			__try
			{
				ProbeForRead(ClientId, 1, 4);
				volatile ULONG64 Touch = (ULONG64)ClientId->UniqueProcess;
				Touch = (ULONG64)ClientId->UniqueThread;
			}

			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return GetExceptionCode();
			}

			if (ClientId->UniqueProcess == NULL)
				return OriginalNtOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);

			PEPROCESS TargetProcess = PidToProcess(ClientId->UniqueProcess);
			UNICODE_STRING ProcessImageName = PsQueryFullProcessImageName(TargetProcess);

			if (Hider::IsProcessNameBad(&ProcessImageName) == TRUE)
			{
				HANDLE OldPid = ClientId->UniqueProcess;

				ClientId->UniqueProcess = UlongToHandle(0xFFFFFFFC);

				NTSTATUS Status = OriginalNtOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);

				ClientId->UniqueProcess = OldPid;

				return Status;
			}
		}
	}
	return OriginalNtOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
}

NTSTATUS(NTAPI* OriginalNtOpenThread)(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);
NTSTATUS NTAPI HookedNtOpenThread(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId)
{
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_OPEN_THREAD) == TRUE &&
		ExGetPreviousMode() == UserMode)
	{
		__try
		{
			ProbeForWrite(ProcessHandle, 4, 1);
			ProbeForWrite(ObjectAttributes, 28, 4);
		}

		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return GetExceptionCode();
		}

		if (ClientId != NULL)
		{
			__try
			{
				ProbeForRead(ClientId, 1, 4);
				volatile ULONG64 Touch = (ULONG64)ClientId->UniqueProcess;
				Touch = (ULONG64)ClientId->UniqueThread;
			}

			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return GetExceptionCode();
			}

			if (ClientId->UniqueThread == NULL)
				return OriginalNtOpenThread(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);

			PETHREAD TargetThread;
			PsLookupThreadByThreadId(ClientId->UniqueThread, &TargetThread);
			if (TargetThread != NULL)
			{
				PEPROCESS TargetProcess = IoThreadToProcess(TargetThread);
				UNICODE_STRING ProcessImageName = PsQueryFullProcessImageName(TargetProcess);

				if (Hider::IsProcessNameBad(&ProcessImageName) == TRUE)
				{
					HANDLE OriginalTID = ClientId->UniqueThread;
					ClientId->UniqueThread = UlongToHandle(0xFFFFFFFC);

					NTSTATUS Status = OriginalNtOpenThread(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);

					ClientId->UniqueThread = OriginalTID;

					return Status;
				}
			}
		}
	}
	return OriginalNtOpenThread(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
}

NTSTATUS(NTAPI* OriginalNtCreateFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes,
	ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
NTSTATUS NTAPI HookedNtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes,
	ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength)
{
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_CREATE_FILE) == TRUE &&
		ExGetPreviousMode() == UserMode
		)
	{
		NTSTATUS Status = OriginalNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);

		if (NT_SUCCESS(Status) == TRUE)
		{
			__try
			{
				UNICODE_STRING SymLink;
				RtlInitUnicodeString(&SymLink, ObjectAttributes->ObjectName->Buffer);

				if (Hider::IsDriverHandleHidden(&SymLink) == TRUE)
				{
					ObCloseHandle(*FileHandle, UserMode);
					*FileHandle = INVALID_HANDLE_VALUE;
					Status = STATUS_OBJECT_NAME_NOT_FOUND;
				}
			}

			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}
		}

		return Status;
	}

	return OriginalNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

NTSTATUS(NTAPI* OriginalNtCreateThreadEx)
(
	PHANDLE ThreadHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	HANDLE ProcessHandle,
	PVOID StartRoutine,
	PVOID Argument,
	ULONG CreateFlags,
	SIZE_T ZeroBits,
	SIZE_T StackSize,
	SIZE_T MaximumStackSize,
	PVOID AttributeList
	);
NTSTATUS NTAPI HookedNtCreateThreadEx
(
	PHANDLE ThreadHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	HANDLE ProcessHandle,
	PVOID StartRoutine,
	PVOID Argument,
	ULONG CreateFlags,
	SIZE_T ZeroBits,
	SIZE_T StackSize,
	SIZE_T MaximumStackSize,
	PVOID AttributeList
)
{
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_CREATE_THREAD_EX) == TRUE &&
		(CreateFlags & THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER ||
			CreateFlags & THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE))
	{
		NTSTATUS Status;
		ULONG OriginalFlags = CreateFlags;

		if (g_HyperHide.CurrentWindowsBuildNumber >= WINDOWS_10_VERSION_19H1)
			Status = OriginalNtCreateThreadEx(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags & ~(THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER | THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE), ZeroBits, StackSize, MaximumStackSize, AttributeList);

		else
			Status = OriginalNtCreateThreadEx(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags & ~(THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER), ZeroBits, StackSize, MaximumStackSize, AttributeList);

		if (NT_SUCCESS(Status) == TRUE)
		{
			PETHREAD NewThread;
			NTSTATUS ObStatus = ObReferenceObjectByHandle(*ThreadHandle, NULL, *PsThreadType, KernelMode, (PVOID*)&NewThread, NULL);

			if (NT_SUCCESS(ObStatus) == TRUE)
			{
				PEPROCESS TargetProcess;
				ObStatus = ObReferenceObjectByHandle(ProcessHandle, NULL, *PsProcessType, KernelMode, (PVOID*)&TargetProcess, NULL);

				if (NT_SUCCESS(ObStatus) == TRUE)
				{
					if (Hider::IsHidden(TargetProcess, HIDE_NT_CREATE_THREAD_EX) == TRUE)
					{
						Hider::PHIDDEN_THREAD HiddenThread = Hider::AppendThreadList(TargetProcess, NewThread);
						if (HiddenThread != NULL)
							HiddenThread->IsThreadHidden = OriginalFlags & THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER;
					}
					ObDereferenceObject(TargetProcess);
				}
				ObDereferenceObject(NewThread);
			}
		}

		return Status;
	}

	return OriginalNtCreateThreadEx(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
}

NTSTATUS(NTAPI* OriginalNtCreateProcessEx)
(
	OUT PHANDLE     ProcessHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
	IN HANDLE   ParentProcess,
	IN ULONG    Flags,
	IN HANDLE SectionHandle     OPTIONAL,
	IN HANDLE DebugPort     OPTIONAL,
	IN HANDLE ExceptionPort     OPTIONAL,
	IN ULONG  JobMemberLevel
	);
NTSTATUS NTAPI HookedNtCreateProcessEx
(
	OUT PHANDLE     ProcessHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
	IN HANDLE   ParentProcess,
	IN ULONG    Flags,
	IN HANDLE SectionHandle     OPTIONAL,
	IN HANDLE DebugPort     OPTIONAL,
	IN HANDLE ExceptionPort     OPTIONAL,
	IN ULONG  JobMemberLevel
)
{
	NTSTATUS Status = OriginalNtCreateProcessEx(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_CREATE_PROCESS_EX) == TRUE &&
		NT_SUCCESS(Status) == TRUE)
	{
		PEPROCESS NewProcess;
		NTSTATUS ObStatus = ObReferenceObjectByHandle(*ProcessHandle, NULL, *PsProcessType, KernelMode, (PVOID*)&NewProcess, NULL);
		if (NT_SUCCESS(ObStatus) == TRUE)
		{
			Hider::PHIDDEN_PROCESS HiddenProcess = Hider::QueryHiddenProcess(IoGetCurrentProcess());
			Hider::CreateEntry(HiddenProcess->DebuggerProcess, NewProcess);

			HIDE_INFO HideInfo = { 0 };

			RtlFillBytes(&HideInfo.HookNtQueryInformationProcess, 1, sizeof(HideInfo) - 4);
			HideInfo.Pid = HandleToUlong(PsGetProcessId(NewProcess));

			Hider::Hide(&HideInfo);
			ObDereferenceObject(NewProcess);
		}
	}
	return Status;
}

NTSTATUS(NTAPI* OriginalNtCreateUserProcess)
(
	PHANDLE ProcessHandle,
	PHANDLE ThreadHandle,
	ACCESS_MASK ProcessDesiredAccess,
	ACCESS_MASK ThreadDesiredAccess,
	POBJECT_ATTRIBUTES ProcessObjectAttributes,
	POBJECT_ATTRIBUTES ThreadObjectAttributes,
	ULONG ProcessFlags,
	ULONG ThreadFlags,
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
	PVOID CreateInfo, // PPS_CREATE_INFO
	PVOID AttributeList // PPS_ATTRIBUTE_LIST
	);

NTSTATUS NTAPI HookedNtCreateUserProcess
(
	PHANDLE ProcessHandle,
	PHANDLE ThreadHandle,
	ACCESS_MASK ProcessDesiredAccess,
	ACCESS_MASK ThreadDesiredAccess,
	POBJECT_ATTRIBUTES ProcessObjectAttributes,
	POBJECT_ATTRIBUTES ThreadObjectAttributes,
	ULONG ProcessFlags,
	ULONG ThreadFlags,
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
	PVOID CreateInfo, // PPS_CREATE_INFO
	PVOID AttributeList // PPS_ATTRIBUTE_LIST
)
{
	NTSTATUS Status = OriginalNtCreateUserProcess
	(
		ProcessHandle, ThreadHandle,
		ProcessDesiredAccess, ThreadDesiredAccess,
		ProcessObjectAttributes, ThreadObjectAttributes,
		ProcessFlags, ThreadFlags,
		ProcessParameters, CreateInfo, AttributeList
	);

	PEPROCESS CurrentProcess = IoGetCurrentProcess();
	if (Hider::IsHidden(CurrentProcess, HIDE_NT_CREATE_PROCESS_EX) == TRUE &&
		ExGetPreviousMode() == UserMode &&
		NT_SUCCESS(Status) == TRUE)
	{
		PEPROCESS NewProcess;
		NTSTATUS ObStatus = ObReferenceObjectByHandle(*ProcessHandle, NULL, *PsProcessType, KernelMode, (PVOID*)&NewProcess, NULL);
		if (NT_SUCCESS(ObStatus) == TRUE)
		{
			Hider::PHIDDEN_PROCESS HiddenProcess = Hider::QueryHiddenProcess(CurrentProcess);
			if (HiddenProcess != NULL)
			{
				HIDE_INFO HideInfo = { 0 };

				Hider::CreateEntry(HiddenProcess->DebuggerProcess, NewProcess);

				RtlFillBytes(&HideInfo.HookNtQueryInformationProcess, 1, sizeof(HideInfo) - 4);
				HideInfo.Pid = HandleToUlong(PsGetProcessId(NewProcess));

				Hider::Hide(&HideInfo);
			}

			ObDereferenceObject(NewProcess);
		}
	}


	return Status;
}

NTSTATUS(NTAPI* OriginalNtYieldExecution)();
NTSTATUS NTAPI HookedNtYieldExecution()
{
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_YIELD_EXECUTION) == TRUE)
	{
		OriginalNtYieldExecution();
		return STATUS_SUCCESS;
	}

	return OriginalNtYieldExecution();
}

NTSTATUS(NTAPI* OriginalNtQuerySystemTime)(PLARGE_INTEGER SystemTime);
NTSTATUS NTAPI HookedNtQuerySystemTime(PLARGE_INTEGER SystemTime)
{
	PEPROCESS Current = IoGetCurrentProcess();

	if (Hider::IsHidden(Current, HIDE_NT_QUERY_SYSTEM_TIME) == TRUE &&
		ExGetPreviousMode() == UserMode)
	{
		__try
		{
			ProbeForWrite(SystemTime, sizeof(ULONG64), 4);

			Hider::PHIDDEN_PROCESS HiddenProcess = Hider::QueryHiddenProcess(Current);
			if (HiddenProcess != NULL)
			{
				if (Hider::IsHidden(Current, HIDE_KUSER_SHARED_DATA) == TRUE)
					SystemTime->QuadPart = *(ULONG64*)&HiddenProcess->Kusd.KuserSharedData->SystemTime;

				else
				{
					if (HiddenProcess->FakeSystemTime.QuadPart == NULL)
						KeQuerySystemTime(&HiddenProcess->FakeSystemTime);

					SystemTime->QuadPart = HiddenProcess->FakeSystemTime.QuadPart;
					HiddenProcess->FakeSystemTime.QuadPart += 1;
				}

				return STATUS_SUCCESS;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return GetExceptionCode();
		}
	}

	return OriginalNtQuerySystemTime(SystemTime);
}

NTSTATUS(NTAPI* OriginalNtQueryPerformanceCounter)(PLARGE_INTEGER PerformanceCounter, PLARGE_INTEGER PerformanceFrequency);
NTSTATUS NTAPI HookedNtQueryPerformanceCounter(PLARGE_INTEGER PerformanceCounter, PLARGE_INTEGER PerformanceFrequency)
{
	PEPROCESS Current = IoGetCurrentProcess();

	if (Hider::IsHidden(Current, HIDE_NT_QUERY_SYSTEM_TIME) == TRUE &&
		ExGetPreviousMode() == UserMode
		)
	{
		__try
		{
			ProbeForWrite(PerformanceCounter, sizeof(ULONG64), 4);
			if (PerformanceFrequency != NULL)
			{
				ProbeForWrite(PerformanceFrequency, sizeof(ULONG64), 4);
			}

			Hider::PHIDDEN_PROCESS HiddenProcess = Hider::QueryHiddenProcess(Current);
			if (HiddenProcess != NULL)
			{
				if (Hider::IsHidden(Current, HIDE_KUSER_SHARED_DATA) == TRUE)
					PerformanceCounter->QuadPart = HiddenProcess->Kusd.KuserSharedData->BaselineSystemTimeQpc;

				else
				{
					if (HiddenProcess->FakePerformanceCounter.QuadPart == NULL)
						HiddenProcess->FakePerformanceCounter = KeQueryPerformanceCounter(NULL);

					PerformanceCounter->QuadPart = HiddenProcess->FakePerformanceCounter.QuadPart;
					HiddenProcess->FakePerformanceCounter.QuadPart += 1;
				}

				if (PerformanceFrequency != NULL)
					PerformanceFrequency->QuadPart = KuserSharedData->QpcFrequency;

				return STATUS_SUCCESS;
			}
		}

		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return GetExceptionCode();
		}
	}

	return OriginalNtQueryPerformanceCounter(PerformanceCounter, PerformanceFrequency);
}

NTSTATUS(NTAPI* OriginalNtContinue)(PCONTEXT Context, ULONG64 TestAlert);
NTSTATUS NTAPI HookedNtContinue(PCONTEXT Context, ULONG64 TestAlert)
{
	PEPROCESS CurrentProcess = IoGetCurrentProcess();
	if (Hider::IsHidden(CurrentProcess, HIDE_NT_CONTINUE) == TRUE &&
		ExGetPreviousMode() == UserMode)
	{
		__try
		{
			ProbeForRead(Context, 1, 16);

			Hider::PHIDDEN_THREAD HiddenThread = Hider::AppendThreadList(CurrentProcess, (PETHREAD)KeGetCurrentThread());

			if ((Context->Dr0 != __readdr(0) && Context->Dr1 != __readdr(1) &&
				Context->Dr2 != __readdr(2) && Context->Dr3 != __readdr(3) &&
				Context->ContextFlags & 0x10 && HiddenThread != NULL) == TRUE)
			{
				RtlCopyBytes(&HiddenThread->FakeDebugContext.DR0, &Context->Dr0, sizeof(ULONG64) * 6);
				RtlCopyBytes(&HiddenThread->FakeDebugContext.DebugControl, &Context->DebugControl, sizeof(ULONG64) * 5);
			}

			Context->ContextFlags &= ~0x10;

			return OriginalNtContinue(Context, TestAlert);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return GetExceptionCode();
		}
	}

	return OriginalNtContinue(Context, TestAlert);
}

NTSTATUS(NTAPI* OriginalNtQueryInformationJobObject)(HANDLE JobHandle, JOBOBJECTINFOCLASS JobInformationClass, PVOID JobInformation, ULONG JobInformationLength, PULONG ReturnLength);
NTSTATUS NTAPI HookedNtQueryInformationJobObject(HANDLE JobHandle, JOBOBJECTINFOCLASS JobInformationClass, PVOID JobInformation, ULONG JobInformationLength, PULONG ReturnLength)
{
	NTSTATUS Status = OriginalNtQueryInformationJobObject(JobHandle, JobInformationClass, JobInformation, JobInformationLength, ReturnLength);
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_QUERY_INFORMATION_JOB_OBJECT) == TRUE &&
		JobInformationClass == JobObjectBasicProcessIdList &&
		NT_SUCCESS(Status) == TRUE)
	{
		BACKUP_RETURNLENGTH();

		PJOBOBJECT_BASIC_PROCESS_ID_LIST JobProcessIdList = (PJOBOBJECT_BASIC_PROCESS_ID_LIST)JobInformation;
		for (size_t i = 0; i < JobProcessIdList->NumberOfAssignedProcesses; i++)
		{
			if (Hider::IsDebuggerProcess(PidToProcess(JobProcessIdList->ProcessIdList[i])) == TRUE)
			{
				if (i == JobProcessIdList->NumberOfAssignedProcesses - 1)
					JobProcessIdList->ProcessIdList[i] = NULL;

				else
				{
					for (size_t j = i + 1; j < JobProcessIdList->NumberOfAssignedProcesses; j++)
					{
						JobProcessIdList->ProcessIdList[j - 1] = JobProcessIdList->ProcessIdList[j];
						JobProcessIdList->ProcessIdList[j] = 0;
					}
				}

				JobProcessIdList->NumberOfAssignedProcesses--;
				JobProcessIdList->NumberOfProcessIdsInList--;
			}
		}

		RESTORE_RETURNLENGTH();
	}
	return Status;
}

// Win32k Syscalls

HANDLE(NTAPI* OriginalNtUserQueryWindow)(HANDLE hWnd, WINDOWINFOCLASS WindowInfo);
HANDLE NTAPI HookedNtUserQueryWindow(HANDLE hWnd, WINDOWINFOCLASS WindowInfo)
{
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_USER_QUERY_WINDOW) == TRUE &&
		(WindowInfo == WindowProcess || WindowInfo == WindowThread) &&
		IsWindowBad(hWnd))
	{
		if (WindowInfo == WindowProcess)
			return PsGetCurrentProcessId();

		if (WindowInfo == WindowThread)
			return PsGetCurrentProcessId();
	}
	return OriginalNtUserQueryWindow(hWnd, WindowInfo);
}

NTSTATUS(NTAPI* OriginalNtUserBuildHwndList)(HANDLE hDesktop, HANDLE hwndParent, BOOLEAN bChildren, BOOLEAN bUnknownFlag, ULONG dwThreadId, ULONG lParam, PHANDLE pWnd, PULONG pBufSize);
NTSTATUS NTAPI HookedNtUserBuildHwndList(HANDLE hDesktop, HANDLE hwndParent, BOOLEAN bChildren, BOOLEAN bUnknownFlag, ULONG dwThreadId, ULONG lParam, PHANDLE pWnd, PULONG pBufSize)
{
	NTSTATUS Status = OriginalNtUserBuildHwndList(hDesktop, hwndParent, bChildren, bUnknownFlag, dwThreadId, lParam, pWnd, pBufSize);
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_USER_BUILD_HWND_LIST) == TRUE &&
		NT_SUCCESS(Status) == TRUE &&
		pWnd != NULL &&
		pBufSize != NULL)
	{
		for (size_t i = 0; i < *pBufSize; i++)
		{
			if (pWnd[i] != NULL && IsWindowBad(pWnd[i]) == TRUE)
			{
				if (i == *pBufSize - 1)
				{
					pWnd[i] = NULL;
					*pBufSize -= 1;
					continue;
				}

				for (size_t j = i + 1; j < *pBufSize; j++)
				{
					pWnd[i] = pWnd[j];
				}

				pWnd[*pBufSize - 1] = NULL;
				*pBufSize -= 1;
				continue;
			}
		}
	}

	return Status;
}

NTSTATUS(NTAPI* OriginalNtUserBuildHwndListSeven)(HANDLE hDesktop, HANDLE hwndParent, BOOLEAN bChildren, ULONG dwThreadId, ULONG lParam, PHANDLE pWnd, PULONG pBufSize);
NTSTATUS NTAPI HookedNtUserBuildHwndListSeven(HANDLE hDesktop, HANDLE hwndParent, BOOLEAN bChildren, ULONG dwThreadId, ULONG lParam, PHANDLE pWnd, PULONG pBufSize)
{
	NTSTATUS Status = OriginalNtUserBuildHwndListSeven(hDesktop, hwndParent, bChildren, dwThreadId, lParam, pWnd, pBufSize);

	PEPROCESS Current = IoGetCurrentProcess();
	if (Hider::IsHidden(Current, HIDE_NT_USER_BUILD_HWND_LIST) == TRUE &&
		NT_SUCCESS(Status) == TRUE &&
		pWnd != NULL &&
		pBufSize != NULL)
	{
		for (size_t i = 0; i < *pBufSize; i++)
		{
			if (pWnd[i] != NULL && IsWindowBad(pWnd[i]) == TRUE)
			{
				if (i == *pBufSize - 1)
				{
					pWnd[i] = NULL;
					*pBufSize -= 1;
					break;
				}

				for (size_t j = i + 1; j < *pBufSize; j++)
				{
					pWnd[i] = pWnd[j];
				}

				pWnd[*pBufSize - 1] = NULL;
				*pBufSize -= 1;
				break;
			}
		}
	}

	return Status;
}

HANDLE(NTAPI* OriginalNtUserFindWindowEx)(PVOID hwndParent, PVOID hwndChild, PUNICODE_STRING ClassName, PUNICODE_STRING WindowName, ULONG Type);
HANDLE NTAPI HookedNtUserFindWindowEx(PVOID hwndParent, PVOID hwndChild, PUNICODE_STRING ClassName, PUNICODE_STRING WindowName, ULONG Type)
{
	HANDLE hWnd = OriginalNtUserFindWindowEx(hwndParent, hwndChild, ClassName, WindowName, Type);
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_USER_FIND_WINDOW_EX) == TRUE &&
		hWnd != NULL)
	{
		if (Hider::IsProcessWindowBad(WindowName) == TRUE || Hider::IsProcessWindowClassBad(ClassName) == TRUE)
			return 0;
	}

	return hWnd;
}

HANDLE(NTAPI* OriginalNtUserGetForegroundWindow)();
HANDLE NTAPI HookedNtUserGetForegroundWindow()
{
	HANDLE hWnd = OriginalNtUserGetForegroundWindow();
	if (Hider::IsHidden(IoGetCurrentProcess(), HIDE_NT_USER_GET_FOREGROUND_WINDOW) == TRUE &&
		hWnd != NULL && IsWindowBad(hWnd) == TRUE)
	{
		hWnd = NtUserGetThreadState(THREADSTATE_ACTIVEWINDOW);
	}

	return hWnd;
}

VOID(NTAPI* OriginalKiDispatchException)(PEXCEPTION_RECORD ExceptionRecord, PKEXCEPTION_FRAME ExceptionFrame, PKTRAP_FRAME TrapFrame, KPROCESSOR_MODE PreviousMode, BOOLEAN FirstChance);
VOID NTAPI HookedKiDispatchException(PEXCEPTION_RECORD ExceptionRecord, PKEXCEPTION_FRAME ExceptionFrame, PKTRAP_FRAME TrapFrame, KPROCESSOR_MODE PreviousMode, BOOLEAN FirstChance)
{
	OriginalKiDispatchException(ExceptionRecord, ExceptionFrame, TrapFrame, PreviousMode, FirstChance);

	PEPROCESS CurrentProcess = IoGetCurrentProcess();//!!!!!!!
	if (PreviousMode == UserMode && TrapFrame->Rip == KiUserExceptionDispatcherAddress && Hider::IsHidden(CurrentProcess, HIDE_KI_EXCEPTION_DISPATCH) == TRUE)
	{
		PETHREAD CurentThread = (PETHREAD)KeGetCurrentThread();
		Hider::PHIDDEN_THREAD HiddenThread = Hider::AppendThreadList(CurrentProcess, CurentThread);

		PCONTEXT UserModeContext = (PCONTEXT)TrapFrame->Rsp;

		if (HiddenThread != NULL)
		{
			if (PsGetProcessWow64Process(CurrentProcess) == NULL)
			{
				RtlCopyBytes(&UserModeContext->Dr0, &HiddenThread->FakeDebugContext.DR0, sizeof(ULONG64) * 6);
				RtlCopyBytes(&UserModeContext->DebugControl, &HiddenThread->FakeDebugContext.DebugControl, sizeof(ULONG64) * 5);
				DbgPrint("RtlCopyBytes \n");
			}

			else
			{
				UserModeContext->Dr0 = HiddenThread->FakeWow64DebugContext.DR0;
				UserModeContext->Dr1 = HiddenThread->FakeWow64DebugContext.DR1;
				UserModeContext->Dr2 = HiddenThread->FakeWow64DebugContext.DR2;
				UserModeContext->Dr3 = HiddenThread->FakeWow64DebugContext.DR3;
				UserModeContext->Dr6 = HiddenThread->FakeWow64DebugContext.DR6;
				UserModeContext->Dr7 = HiddenThread->FakeWow64DebugContext.DR7;

				RtlSecureZeroMemory(&TrapFrame->DebugControl, sizeof(ULONG64) * 5);
				DbgPrint("RtlSecureZeroMemory \n");
			}
		}
	}
}


NTSTATUS   KeReadMemory(IN HANDLE ProcessHandle, IN PVOID AddressToRead, IN ULONG LenthToRead, IN OUT PVOID BufferToRecviveData)
{
	NTSTATUS    status = STATUS_UNSUCCESSFUL;
	PEPROCESS   ProcessPointer = NULL;
	BOOLEAN     IsAttached = FALSE;
	__try
	{

		status = ObReferenceObjectByHandle((HANDLE)ProcessHandle,
			PROCESS_VM_READ,
			*PsProcessType,
			KernelMode,
			(PVOID*)&ProcessPointer,
			NULL);
		if (!NT_SUCCESS(status))
		{
			__leave;
		}
		ObDereferenceObject(ProcessPointer);
		IsAttached = TRUE;

		KeAttachProcess((PRKPROCESS)ProcessPointer);
		if (!MmIsAddressValid(AddressToRead))
			status = STATUS_INVALID_PARAMETER;
		else
			RtlCopyMemory(BufferToRecviveData, (PVOID)AddressToRead, LenthToRead);

	}
	__finally
	{
		if (IsAttached)
		{
			KeDetachProcess();
			IsAttached = FALSE;
		}
	}
	return status;
}

#define ProbeForWriteUlong_ptr(Address) {                                    \
    if ((Address) >= (ULONG_PTR * const)MM_USER_PROBE_ADDRESS) {             \
        *(volatile ULONG_PTR * const)MM_USER_PROBE_ADDRESS = 0;              \
    }                                                                        \
                                                                             \
    *(volatile ULONG_PTR *)(Address) = *(volatile ULONG_PTR *)(Address);     \
}

NTSTATUS
myNtReadVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	OUT PULONG NumberOfBytesRead OPTIONAL
)
{
	SIZE_T BytesCopied = 0;
	KPROCESSOR_MODE PreviousMode;
	PEPROCESS Process;
	NTSTATUS Status;

	PAGED_CODE();

	PreviousMode = ExGetPreviousMode();
	if (PreviousMode != KernelMode) {
		if (((PCHAR)BaseAddress + BufferSize < (PCHAR)BaseAddress) ||
			((PCHAR)Buffer + BufferSize < (PCHAR)Buffer) ||
			((PVOID)((PCHAR)BaseAddress + BufferSize) > MM_HIGHEST_USER_ADDRESS) ||
			((PVOID)((PCHAR)Buffer + BufferSize) > MM_HIGHEST_USER_ADDRESS)) {

			return STATUS_ACCESS_VIOLATION;
		}

		if (ARGUMENT_PRESENT(NumberOfBytesRead)) {
			__try {
				ProbeForWriteUlong_ptr((PULONG_PTR)NumberOfBytesRead);

			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return GetExceptionCode();
			}
		}
	}

	BytesCopied = 0;
	Status = STATUS_SUCCESS;
	if (BufferSize != 0) {

		Status = ObReferenceObjectByHandle(ProcessHandle,
			PROCESS_VM_READ,
			*PsProcessType,
			PreviousMode,
			(PVOID*)&Process,
			NULL);

		if (Status == STATUS_SUCCESS) {
			Status = MmCopyVirtualMemory(Process,
				BaseAddress,
				PsGetCurrentProcess(),
				Buffer,
				BufferSize,
				PreviousMode,
				&BytesCopied);

			ObDereferenceObject(Process);
		}
	}

	if (ARGUMENT_PRESENT(NumberOfBytesRead)) {
		__try {
			*NumberOfBytesRead = BytesCopied;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NOTHING;
		}
	}

	return Status;
}

NTSTATUS myNtReadVirtualMemory1(IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	OUT PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead)
{
	NTSTATUS Status;
	PMDL Mdl;
	PVOID SystemAddress;
	PEPROCESS Process;

	DbgPrint("NtReadVirtualMemory(ProcessHandle %x, BaseAddress %x, "
		"Buffer %x, NumberOfBytesToRead %d)\n", ProcessHandle, BaseAddress,
		Buffer, NumberOfBytesToRead);

	Status = ObReferenceObjectByHandle(ProcessHandle,
		PROCESS_VM_WRITE,
		NULL,
		UserMode,
		(PVOID*)(&Process),
		NULL);

	if (Status != STATUS_SUCCESS)
	{
		return(Status);
	}

	Mdl = MmCreateMdl(NULL, Buffer, NumberOfBytesToRead);
	MmProbeAndLockPages(Mdl, UserMode, IoWriteAccess);
	KeAttachProcess(Process);
	SystemAddress = MmGetSystemAddressForMdl(Mdl);
	memcpy(SystemAddress, BaseAddress, NumberOfBytesToRead);
	KeDetachProcess();
	if (Mdl->MappedSystemVa != NULL)
	{
		MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
	}
	MmUnlockPages(Mdl);
	ExFreePool(Mdl);

	ObDereferenceObject(Process);

	memcpy(Buffer, BaseAddress, NumberOfBytesToRead);
	*NumberOfBytesRead = NumberOfBytesToRead;
	return(STATUS_SUCCESS);
}


//VOID KeAttachProcess(PEPROCESS Process)
//{
//	KIRQL oldlvl;
//	PETHREAD_S CurrentThread;
//	PULONG AttachedProcessPageDir;
//	ULONG PageDir;
//
//	DbgPrint("KeAttachProcess(Process %x)\n", Process);
//
//	CurrentThread = PsGetCurrentThread();
//
//	if (CurrentThread->OldProcess != NULL)
//	{
//		DbgPrint("Invalid attach (thread is already attached)\n");
//		KeBugCheck(0);
//	}
//
//	KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);
//
//	KiSwapApcEnvironment(&CurrentThread->Tcb, &Process->Pcb);
//	/* The stack of the current process may be located in a page which is
//		 not present in the page directory of the process we're attaching to.
//		 That would lead to a page fault when this function returns. However,
//		 since the processor can't call the page fault handler 'cause it can't
//		 push EIP on the stack, this will show up as a stack fault which will
//		 crash the entire system.
//		 To prevent this, make sure the page directory of the process we're
//		 attaching to is up-to-date. */
//
//	AttachedProcessPageDir = ExAllocatePageWithPhysPage(Process->Pcb.DirectoryTableBase);
//	MmUpdateStackPageDir(AttachedProcessPageDir, &CurrentThread->Tcb);
//	ExUnmapPage(AttachedProcessPageDir);
//	CurrentThread->OldProcess = PsGetCurrentProcess();
//	CurrentThread->ThreadsProcess = Process;
//	PageDir = Process->Pcb.DirectoryTableBase.u.LowPart;
//	DbgPrint("Switching process context to %x\n", PageDir);
//	Ke386SetPageTableDirectory(PageDir);
//	KeLowerIrql(oldlvl);
//}

typedef NTSTATUS(*NtReadVirtualMemory_t)(
	IN HANDLE               ProcessHandle,
	IN PVOID                BaseAddress,
	IN PVOID                Buffer,
	IN ULONG                NumberOfBytesToRead,
	OUT PULONG              NumberOfBytesReaded OPTIONAL);
NtReadVirtualMemory_t orignal_NtReadVirtualMemory;
NTSTATUS NTAPI HookNtReadVirtualMemory(IN HANDLE ProcessHandle, IN PVOID BaseAddress, OUT PVOID Buffer, IN ULONG NumberOfBytesToRead, PULONG NumberOfBytesReaded)
{
	NTSTATUS ret = myNtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToRead, NumberOfBytesReaded);
	if (NT_SUCCESS(ret))
	{
		return STATUS_SUCCESS;
	}
	else
	{
		DbgPrint("NtReadVirtualMemory failed:%x(ProcessHandle: %p, BaseAddress: %p, "
			"Buffer: %p, NumberOfBytesToRead %d)\n", ProcessHandle, BaseAddress,
			Buffer, NumberOfBytesToRead, ret);

		return ret;
	}




	//if (ProcessHandle == (HANDLE)-1)
	//{
	//	DbgPrint("ProcessHandle = -1 \n");
	//	return orignal_NtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToRead, NumberOfBytesReaded);
	//}

	//PEPROCESS pTargetProcess;
	//if (NT_SUCCESS(ObReferenceObjectByHandle(ProcessHandle, PROCESS_VM_READ, *PsProcessType, ExGetPreviousMode(), (PVOID*)&pTargetProcess, nullptr)));
	//{
	//	if (IoGetCurrentProcess() == pTargetProcess)
	//	{
	//		DbgPrint("EProcess equal\n");
	//		return orignal_NtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToRead, NumberOfBytesReaded);
	//	}
	//}

	PEPROCESS currProcess;
	NTSTATUS status = PsLookupProcessByProcessId(PsGetCurrentProcessId(), &currProcess);
	if (NT_SUCCESS(status))
	{
		CHAR* Name = (char*)PsGetProcessImageFileName(currProcess);
		//if (_stricmp(Name, "ce32.exe") == 0 || _stricmp(Name, "ce64.exe") == 0 || _stricmp(Name, "ce.exe") == 0
			//|| _stricmp(Name, "x64dbg.exe") == 0)
			//if(PsGetCurrentProcessId() == (HANDLE)3712)
		{
			CHAR* psname = (char*)GetProcessNameFromProcessHandle(ProcessHandle);
			if (psname)
			{
				//if (_stricmp(psname, "tslgame.exe") == 0)
				{
					PEPROCESS popenProcess;
					if (NT_SUCCESS(ObReferenceObjectByHandle(ProcessHandle, PROCESS_VM_READ, *PsProcessType, ExGetPreviousMode(), (PVOID*)&popenProcess, nullptr)))
					{
						//if (NumberOfBytesReaded)
						//{
						//	SIZE_T  BytesCopied = 0;
						//	NTSTATUS ret = MmCopyVirtualMemory(popenProcess, BaseAddress, currProcess, Buffer, NumberOfBytesToRead, ExGetPreviousMode(), (PSIZE_T)&BytesCopied);
						//	if (NT_SUCCESS(ret))
						//	{
						//		//DbgPrint("MmCopyVirtualMemory ok \n");
						//	}
						//	ObDereferenceObject(popenProcess);
						//	ObDereferenceObject(currProcess);
						//	*NumberOfBytesReaded = BytesCopied;
						//	return ret;
						//}
						if (NumberOfBytesReaded)
						{
							SIZE_T size = NumberOfBytesToRead;
							BOOLEAN ret = MyReadMemory(BaseAddress, Buffer, size, NumberOfBytesReaded, popenProcess,0);
							if (ret)
							{
								//DbgPrint("HookNtReadVirtualMemory:MyReadMemory:%d \n", NumberOfBytesToRead);
								//KeStackAttachProcess(PsGetCurrentProcess(), &apc);
								*NumberOfBytesReaded = NumberOfBytesToRead;
								//KeUnstackDetachProcess(&apc);
								ObDereferenceObject(popenProcess);
								return STATUS_SUCCESS;
							}
							else
							{
								DbgPrint("currpid:%p HookNtReadVirtualMemory:MyReadMemory:failed \n", PsGetCurrentProcessId());
								return ret;
							}
						}
						ObDereferenceObject(popenProcess);
					}
					else
					{
						DbgPrint("HookNtReadVirtualMemory:ObReferenceObjectByHandle failed\n");
					}
				}
			}
		}
	}
	//DbgPrint("NumberOfBytesToRead:%lu NumberOfBytesReaded:%lu \n", NumberOfBytesToRead, *NumberOfBytesReaded);
	//return orignal_NtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToRead, NumberOfBytesReaded);

	//NTSTATUS ntsat = orignal_NtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToRead, NumberOfBytesReaded);
	//DbgPrint("orignal_NtReadVirtualMemory:NumberOfBytesToRead:%lu NumberOfBytesReaded:%lu\n", NumberOfBytesToRead, *NumberOfBytesReaded);
	//return ntsat;

}

typedef NTSTATUS(*NtWriteVirtualMemory_t)(
	IN HANDLE               ProcessHandle,
	IN PVOID                BaseAddress,
	IN PVOID                Buffer,
	IN ULONG                NumberOfBytesToWrite,
	OUT PULONG              NumberOfBytesWritten OPTIONAL);
NtWriteVirtualMemory_t orignal_NtWriteVirtualMemory;
NTSTATUS NTAPI HookNtWriteVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToWrite, PULONG NumberOfBytesWritten)
{
	PEPROCESS ps;
	NTSTATUS status = PsLookupProcessByProcessId(PsGetCurrentProcessId(), &ps);
	if (NT_SUCCESS(status))
	{
		char* Name = (char*)PsGetProcessImageFileName(ps);
		if (_stricmp(Name, "ce32.exe") == 0 || _stricmp(Name, "ce64.exe") == 0 || _stricmp(Name, "ce.exe") == 0
			|| _stricmp(Name, "x64dbg.exe") == 0)
		{
			PEPROCESS popenProcess;
			if (NT_SUCCESS(ObReferenceObjectByHandle(ProcessHandle, 0, *PsProcessType, ExGetPreviousMode(), (PVOID*)&popenProcess, nullptr)))
			{
				ObDereferenceObject(popenProcess);
				MyWriteMemory(BaseAddress, Buffer, NumberOfBytesToWrite, ps);
				*NumberOfBytesWritten = NumberOfBytesToWrite;
				return STATUS_SUCCESS;
			}
		}
	}


	/*
	if (g_startDebug)
	{
		ULONG pid = (ULONG)(ULONG_PTR)PsGetCurrentProcessId();
		if (g_DbgPid == pid)
		{
			if (GetProcessIDFromProcessHandle(ProcessHandle) == g_GamePid)
			{
				return pfn_NtWriteVirtualMemory(g_GameHandle, BaseAddress, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);
			}
		}
	}
	*/
	return orignal_NtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);
}

BOOLEAN HookSyscalls()
{
	NT_SYSCALL_NUMBERS SyscallNumbers;
	WIN32K_SYSCALL_NUMBERS Win32KSyscallNumbers;

	KeInitializeMutex(&NtCloseMutex, 0);
	GetNtSyscallNumbers(SyscallNumbers);
	GetWin32kSyscallNumbers(Win32KSyscallNumbers);

	PEPROCESS CsrssProcess = GetCsrssProcess();
	PVOID NtDllAddress = GetUserModeModule(CsrssProcess, L"ntdll.dll", FALSE);

	KiUserExceptionDispatcherAddress = (ULONG64)GetExportedFunctionAddress(CsrssProcess, NtDllAddress, "KiUserExceptionDispatcher");
	if (KiUserExceptionDispatcherAddress == NULL)
	{
		LogError("Couldn't get KiUserExceptionDispatcher address");
		return FALSE;
	}

	LogInfo("KiUserExceptionDispatcher address: 0x%llx", KiUserExceptionDispatcherAddress);

	NtUserGetThreadState = (HANDLE(NTAPI*)(ULONG))SSDT::GetWin32KFunctionAddress("NtUserGetThreadState", Win32KSyscallNumbers.NtUserGetThreadState);
	if (NtUserGetThreadState == NULL)
	{
		LogError("Couldn't get NtUserGetThreadState address");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtContinue, HookedNtContinue, (PVOID*)&OriginalNtContinue) == FALSE)
	{
		LogError("NtContinue hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtSetInformationThread, HookedNtSetInformationThread, (PVOID*)&OriginalNtSetInformationThread) == FALSE)
	{
		LogError("NtSetInformationThread hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtQueryInformationProcess, HookedNtQueryInformationProcess, (PVOID*)&OriginalNtQueryInformationProcess) == FALSE)
	{
		LogError("NtQueryInformationProcess hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtQueryObject, HookedNtQueryObject, (PVOID*)&OriginalNtQueryObject) == FALSE)
	{
		LogError("NtQueryObject hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtSystemDebugControl, HookedNtSystemDebugControl, (PVOID*)&OriginalNtSystemDebugControl) == FALSE)
	{
		LogError("NtSystemDebugControl hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtClose, HookedNtClose, (PVOID*)&OriginalNtClose) == FALSE)
	{
		LogError("NtClose hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtSetContextThread, HookedNtSetContextThread, (PVOID*)&OriginalNtSetContextThread) == FALSE)
	{
		LogError("NtSetContextThread hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtQuerySystemInformation, HookedNtQuerySystemInformation, (PVOID*)&OriginalNtQuerySystemInformation) == FALSE)
	{
		LogError("NtQuerySystemInformation hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtGetContextThread, HookedNtGetContextThread, (PVOID*)&OriginalNtGetContextThread) == FALSE)
	{
		LogError("NtGetContextThread hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtQueryInformationThread, HookedNtQueryInformationThread, (PVOID*)&OriginalNtQueryInformationThread) == FALSE)
	{
		LogError("NtQueryInformationThread hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtCreateThreadEx, HookedNtCreateThreadEx, (PVOID*)&OriginalNtCreateThreadEx) == FALSE)
	{
		LogError("NtCreateThreadEx hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtCreateFile, HookedNtCreateFile, (PVOID*)&OriginalNtCreateFile) == FALSE)
	{
		LogError("NtCreateFile hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtCreateUserProcess, HookedNtCreateUserProcess, (PVOID*)&OriginalNtCreateUserProcess) == FALSE)
	{
		LogError("NtCreateUserProcess hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtCreateProcessEx, HookedNtCreateProcessEx, (PVOID*)&OriginalNtCreateProcessEx) == FALSE)
	{
		LogError("NtCreateProcessEx hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtYieldExecution, HookedNtYieldExecution, (PVOID*)&OriginalNtYieldExecution) == FALSE)
	{
		LogError("NtYieldExecution hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtQuerySystemTime, HookedNtQuerySystemTime, (PVOID*)&OriginalNtQuerySystemTime) == FALSE)
	{
		LogError("NtQuerySystemTime hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtQueryPerformanceCounter, HookedNtQueryPerformanceCounter, (PVOID*)&OriginalNtQueryPerformanceCounter) == FALSE)
	{
		LogError("NtQueryPerformanceCounter hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtQueryInformationJobObject, HookedNtQueryInformationJobObject, (PVOID*)&OriginalNtQueryInformationJobObject) == FALSE)
	{
		LogError("NtQueryInformationJobObject hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtGetNextProcess, HookedNtGetNextProcess, (PVOID*)&OriginalNtGetNextProcess) == FALSE)
	{
		LogError("NtGetNextProcess hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtOpenProcess, HookedNtOpenProcess, (PVOID*)&OriginalNtOpenProcess) == FALSE)
	{
		LogError("NtOpenProcess hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtOpenThread, HookedNtOpenThread, (PVOID*)&OriginalNtOpenThread) == FALSE)
	{
		LogError("NtOpenThread hook failed");
		return FALSE;
	}

	if (SSDT::HookNtSyscall(SyscallNumbers.NtSetInformationProcess, HookedNtSetInformationProcess, (PVOID*)&OriginalNtSetInformationProcess) == FALSE)
	{
		LogError("NtSetInformationProcess hook failed");
		return FALSE;
	}

	//if (SSDT::HookNtSyscall(SyscallNumbers.NtReadVirtualMemory, HookNtReadVirtualMemory, (PVOID*)&orignal_NtReadVirtualMemory) == FALSE)
	//{
	//	LogError("NtReadVirtualMemory hook failed");
	//	return FALSE;
	//}

	//if (SSDT::HookNtSyscall(SyscallNumbers.NtWriteVirtualMemory, HookNtWriteVirtualMemory, (PVOID*)&orignal_NtWriteVirtualMemory) == FALSE)
	//{
	//	LogError("NtWriteVirtualMemory hook failed");
	//	return FALSE;
	//}

	if (SSDT::HookWin32kSyscall("NtUserFindWindowEx", Win32KSyscallNumbers.NtUserFindWindowEx, HookedNtUserFindWindowEx, (PVOID*)&OriginalNtUserFindWindowEx) == FALSE)
	{
		LogError("NtUserFindWindowEx hook failed");
		return FALSE;
	}

	if (SSDT::HookWin32kSyscall("NtUserGetForegroundWindow", Win32KSyscallNumbers.NtUserGetForegroundWindow, HookedNtUserGetForegroundWindow, (PVOID*)&OriginalNtUserGetForegroundWindow) == FALSE)
	{
		LogError("NtUserGetForegroundWindow hook failed");
		return FALSE;
	}

	if (SSDT::HookWin32kSyscall("NtUserQueryWindow", Win32KSyscallNumbers.NtUserQueryWindow, HookedNtUserQueryWindow, (PVOID*)&OriginalNtUserQueryWindow) == FALSE)
	{
		LogError("NtUserQueryWindow hook failed");
		return FALSE;
	}

	if (g_HyperHide.CurrentWindowsBuildNumber <= WINDOWS_7_SP1)
	{
		if (SSDT::HookWin32kSyscall("NtUserBuildHwndList", Win32KSyscallNumbers.NtUserBuildHwndList, HookedNtUserBuildHwndListSeven, (PVOID*)&OriginalNtUserBuildHwndListSeven) == FALSE)
		{
			LogError("NtUserBuildHwndListSeven hook failed");
			return FALSE;
		}
	}

	else
	{
		if (SSDT::HookWin32kSyscall("NtUserBuildHwndList", Win32KSyscallNumbers.NtUserBuildHwndList, HookedNtUserBuildHwndList, (PVOID*)&OriginalNtUserBuildHwndList) == FALSE)
		{
			LogError("NtUserBuildHwndList hook failed");
			return FALSE;
		}
	}
	if (HookKiDispatchException(HookedKiDispatchException, (PVOID*)&OriginalKiDispatchException) == FALSE)
	{
		LogError("KiDispatchException hook failed");
		return FALSE;
	}


	return TRUE;
}