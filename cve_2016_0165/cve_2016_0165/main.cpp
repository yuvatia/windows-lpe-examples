#include <iostream>

#include "helpers.h"

HBITMAP WORKER_BITMAP = 0;
HBITMAP MANAGER_BITMAP = 0;


int main() {
	ULONGLONG ptr = (ULONGLONG)&::PathToRegion;
	std::cout << "Hello world!" << std::endl;
	std::cout << "0x" << std::hex << ptr << std::endl;

	int a = 0;
	std::cin >> a;
	std::cout << a;
	return 0;
}