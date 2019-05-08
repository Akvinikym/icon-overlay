#pragma once

#include <algorithm> // std::copy
#include <iterator>  // std::back_inserter
#include <vector>

#include <Windows.h>

// probably the following not needed
#include <winternl.h>
#pragma comment(lib, "ntdll.lib")

enum class Arch { x86, x64 };

// Copy address to mem respecting Intel endianess
void IntelAddressToMem(
	PUCHAR dest_mem,
	LPVOID address,
	Arch target_arch
)
{
	const size_t loops = Arch::x86 == target_arch ? 4 : 8;
	for (auto i = 0; i < loops; ++i, ++dest_mem) {
		*dest_mem = ((uintptr_t)address >> (8 * i)) & 0xFF;
	}
}

#ifdef _CallShellCode // seems that call code is not needed now
/// Create shell code that does call to specified address
std::vector<UCHAR> CallShellCode(
	LPVOID call_to_address,
	Arch target_arch
)
{
	UCHAR x86_shell_code[] = {
		0xb8,						// mov	eax, 0x0
		0x00, 0x00, 0x00, 0x00,		// ;	0x0 func addr
		0xff, 0xd0					// call	eax
	};
	const size_t x86_length = 7;

	UCHAR x64_shell_code[] = {
		0x48, 0xb8,										// movabs	rax, 0x0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// ;		0x0 x64 func addr
		0xff, 0xd0										// call		rax
	};
	const size_t x64_length = 12;

	size_t length;
	PUCHAR source;
	if (Arch::x86 == target_arch)
	{
		IntelAddressToMem(x86_shell_code + 1, call_to_address, target_arch);
		length = x86_length;
		source = x86_shell_code;
	}
	else
	{
		IntelAddressToMem(x64_shell_code + 2, call_to_address, target_arch);
		length = x64_length;
		source = x64_shell_code;
	}

	std::vector<UCHAR> shell_code;
	shell_code.reserve(length);
	std::copy(source, source + length, std::back_inserter(shell_code));

	return shell_code;
}
#endif


std::vector<UCHAR> MovAddrToRax(
	LPVOID address,
	Arch target_arch
)
{
	UCHAR x86_shell_code[] = {
		0xB8,					// mov	eax, 0x0
		0x00, 0x00, 0x00, 0x00	// ;	0x0 address
	};
	const size_t x86_length = 5;

	UCHAR x64_shell_code[] = {
		0x48, 0xB8,											//	movabs	rax, 0x0
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00      //	;		0x0 x64 address
	};
	const size_t x64_length = 10;

	size_t length;
	PUCHAR source;
	if (Arch::x86 == target_arch)
	{
		IntelAddressToMem(x86_shell_code + 1, address, target_arch);
		length = x86_length;
		source = x86_shell_code;
	}
	else
	{
		IntelAddressToMem(x64_shell_code + 2, address, target_arch);
		length = x64_length;
		source = x64_shell_code;
	}

	std::vector<UCHAR> shell_code;
	shell_code.reserve(length);
	std::copy(source, source + length, std::back_inserter(shell_code));

	return shell_code;
}

/// Create shell code that does jump to specified address
std::vector<UCHAR> JumpShellCode(
	LPVOID jump_to_address,
	Arch target_arch
)
{
	UCHAR x86_shell_code[] = {
		0xe9,					// jmp	0x0
		0x00, 0x00, 0x00, 0x00	// ;	0x0 target address
	};
	const size_t x86_length = 5;

	UCHAR x64_shell_code[] = {
		0x48, 0xff, 0x25, 0x00, 0x00, 0x00, 0x00,		// rex.W jmp QWORD PTR[rip + 0x0]
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // 0x0 x64 address
	};
	const size_t x64_length = 15;

	size_t length; // resulting shell code length
	PUCHAR source;
	if (Arch::x86 == target_arch)
	{
		IntelAddressToMem(x86_shell_code + 1, jump_to_address, target_arch);
		length = x86_length;
		source = x86_shell_code;
	}
	else
	{
		IntelAddressToMem(x64_shell_code + 7, jump_to_address, target_arch);
		length = x64_length;
		source = x64_shell_code;
	}

	std::vector<UCHAR> shell_code;
	shell_code.reserve(length);
	std::copy(source, source + length, std::back_inserter(shell_code));

	return shell_code;
}

/// Get function address by module and function names
LPVOID FindModuleFunction(
	LPCSTR module,
	LPCSTR function
)
{
	return GetProcAddress(
		GetModuleHandle(module),
		function
	);
}

LPVOID AllocExecutableMem(size_t size) {
	return VirtualAllocEx(
		GetCurrentProcess(),
		NULL, // desired starting address
		size,
		MEM_COMMIT,
		PAGE_EXECUTE_READWRITE
	);
}

std::vector<UCHAR> ReadMem(
	LPVOID start_address,
	size_t size
)
{
	std::vector<UCHAR> mem(size);
	size_t bytes_read;
	auto result = ReadProcessMemory(
		GetCurrentProcess(),
		start_address,
		mem.data(),
		size,
		&bytes_read);
	if (!result || bytes_read != size)
	{
		mem.clear();
	}
	return mem;
}

BOOL WriteMem(
	LPVOID start_address,
	std::vector<UCHAR> shell_code
)
{
	size_t bytes_written;
	auto result = WriteProcessMemory(
		GetCurrentProcess(),
		start_address,
		shell_code.data(),
		shell_code.size(),
		&bytes_written
	);
	if (!result || bytes_written != shell_code.size())
	{
		return FALSE;
	}
	return TRUE;
}

/**
* Replaces an implementation of a funtion witin current process
*
* @param module - a name of a module that exports the target function
* @param module_function - a name of the funtion to be substituted
* @replacement_function_addr - an address of a function that will
*                              substitute the original module_funciton
* @param target_arch - program architecture
* @return the new address of the original module_function
*/
LPVOID Patch(
	LPCSTR module,
	LPCSTR module_function,
	LPVOID replacement_funtion_addr,
	Arch target_arch
)
{
#define CHECK(r) if (!(r)) return NULL;
	using ShellCode = std::vector<UCHAR>;
	size_t inc = 0;

	// This shell code will be written to
	// the beginning of the original module fucntion.
	ShellCode jump_to_replacement = JumpShellCode(
		replacement_funtion_addr,
		target_arch
	);

	LPVOID module_func = FindModuleFunction(module, module_function);

	char *address = new char[50];
	sprintf(address, "%p", module_func);
	MessageBox(NULL, address, "HELLO", MB_OK);
	//LPVOID relative_addr = (PUCHAR)module_func + 0x8 + 0x153110; // depends on a build of explorer
	//ShellCode movabs = MovAddrToRax(relative_addr, target_arch);

	ShellCode original_beginning = ReadMem(module_func, jump_to_replacement.size()); // -7 depends on a build of explorer
	//std::copy(movabs.begin(), movabs.end(), std::back_inserter(original_beginning));
	CHECK(!original_beginning.empty());

	// We convert func address to pointer to unsigned char to force
	// pointer arithmetic use 1 byte type as counting unit.
	LPVOID original_continuation_addr = (PUCHAR)module_func + jump_to_replacement.size() + inc;

	ShellCode jump_to_original_cont = JumpShellCode(
		original_continuation_addr,
		target_arch
	);

	LPVOID new_original_beginning_addr = AllocExecutableMem(
		original_beginning.size() + jump_to_replacement.size() + inc
	);
	CHECK(new_original_beginning_addr);

	std::copy(std::begin(jump_to_original_cont), std::end(jump_to_original_cont), std::back_inserter(original_beginning));

	auto written1 = WriteMem(
		new_original_beginning_addr,
		original_beginning
	);
	CHECK(written1);
	//auto written2 = WriteMem(
	//	(PUCHAR)new_original_beginning_addr + jump_to_replacement.size(),
	//	jump_to_original_cont
	//);
	//CHECK(written2);

	//ShellCode read = ReadMem(new_original_beginning_addr, 30);
	//bool equal = std::equal(original_beginning.begin(), original_beginning.end(), read.begin());

	auto written3 = WriteMem(module_func, jump_to_replacement);
	CHECK(written3);

	return new_original_beginning_addr;
}