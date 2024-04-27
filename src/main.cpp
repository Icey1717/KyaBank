#include <string.h>
#include <stdio.h>
#include <iostream>
#include <filesystem>

#include "edBank.h"
#include "edBankBuffer.h"
#include "edBankFile.h"
#include "edFile.h"
#include "edFileFiler.h"
#include "edMem.h"
#include "edSystem.h"
#include "ps2/_edFileFilerCDVD.h"

void Initialize()
{
	edCSysHandlerPool::initialize(&g_SysHandlersNodeTable_00489170, 0x8c);

	edFileInit();

	edFileSetPath("<cdvd>");

	/* <cdvd> */
	int local_4 = 0;
	bool bVar1 = edFileFilerConfigure("<cdvd>", IM_CALC_SIZE, (void*)0x898, &local_4);
	if (bVar1 != false) {
		edCdlFolder* PTR_edCdlFolder_00448ef4 = (edCdlFolder*)edMemAlloc(TO_HEAP(H_MAIN), local_4);
		/* <cdvd> */
		edFileFilerConfigure("<cdvd>", IM_INIT, PTR_edCdlFolder_00448ef4, (int*)local_4);
	}

	edBankInit();
}

void ListFiles(const char* pBankPath)
{
	printf("Listing %s:\n", pBankPath);

	edCBankBuffer BootData_BankBuffer;
	edCBankBufferEntry* BootData_BankBufferEntry;

	edCBankInstall bankHeader;
	memset(&bankHeader, 0, sizeof(edCBankInstall));

	BootData_BankBuffer.initialize(0x3200000, 1, &bankHeader);
	/* Set the bank header to point towards 'CDEURO/menu/Messages.bnk' */
	bankHeader.filePath = const_cast<char*>(pBankPath);
	BootData_BankBufferEntry = BootData_BankBuffer.get_free_entry();
	BootData_BankBufferEntry->load(&bankHeader);

	int elementCount = BootData_BankBufferEntry->get_element_count();

	for (int i = 0; i < elementCount; i++) {
		printf("%s\n", DebugFindFilePath(BootData_BankBufferEntry->pFileHeader, i));
	}

	BootData_BankBufferEntry->close();
	BootData_BankBuffer.terminate();

	printf("\n");
}

int main() 
{
	Initialize();

	// Request the directory from the user
	char directory[256];
	std::cout << "Enter the directory: ";
	std::cin >> directory;

	// Find all paths to .bnk files in a directory
	std::filesystem::path p = directory;
	std::filesystem::recursive_directory_iterator it(p);
	for (auto& entry : it) {
		if (entry.path().extension() == ".bnk" || entry.path().extension() == ".BNK") {
			ListFiles(entry.path().string().c_str());
		}
	}

	return 0;
}