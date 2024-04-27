#include <string.h>
#include <stdio.h>
#include <iostream>
#include <filesystem>

#include <argparse/argparse.hpp>

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
		edCdlFolder* pCdlFolder = (edCdlFolder*)edMemAlloc(TO_HEAP(H_MAIN), local_4);
		edFileFilerConfigure("<cdvd>", IM_INIT, pCdlFolder, (int*)local_4);
	}

	edBankInit();
}

class LoadedBank
{
	edCBankBuffer bankBuffer;

public:
	edCBankBufferEntry* pBankBufferEntry;

	LoadedBank(const char* pBankPath)
		: bankBuffer()
		, pBankBufferEntry(nullptr)
	{
		edCBankInstall bankHeader;
		memset(&bankHeader, 0, sizeof(edCBankInstall));

		bankBuffer.initialize(0x3200000, 1, &bankHeader);
		bankHeader.filePath = const_cast<char*>(pBankPath);
		pBankBufferEntry = bankBuffer.get_free_entry();
		pBankBufferEntry->load(&bankHeader);
	}

	~LoadedBank()
	{
		pBankBufferEntry->close();
		bankBuffer.terminate();
	}
	
	LoadedBank(const LoadedBank&) = delete;
	LoadedBank& operator=(const LoadedBank&) = delete;
};

void ListFiles(const char* pBankPath)
{
	printf("Listing Files contained in %s:\n", pBankPath);

	LoadedBank loadedBank(pBankPath);

	int elementCount = loadedBank.pBankBufferEntry->get_element_count();

	for (int i = 0; i < elementCount; i++) {
		printf("%s\n", DebugFindFilePath(loadedBank.pBankBufferEntry->pFileHeader, i));
	}
}

void ExtractFiles(const char* pBankPath, const char* pOutputPath)
{
	printf("Extracting Files contained in %s:\n", pBankPath);
	printf("To: %s\n\n", pOutputPath);

	LoadedBank loadedBank(pBankPath);

	edCBankFileHeader* pFileHeader = loadedBank.pBankBufferEntry->pFileHeader;

	int inIndex = 0;
	if (pFileHeader->fileCount != 0) {
		do {
			const int fileDataIndex = pFileHeader->get_index(inIndex, 0);
			const std::string fileName = pFileHeader->get_entry_filename(inIndex);

			printf("Extracting: %s\n", fileName.c_str());
			const FileHeaderFileData* pFileData = pFileHeader->get_entry(fileDataIndex);

			printf("Size: %x\n", pFileData->size);
			printf("Offset: %x\n", pFileData->offset);

			const char* pStart = reinterpret_cast<char*>(pFileHeader) + pFileData->offset - 8;
			const char* pEnd = pStart + pFileData->size;

			if (!std::filesystem::exists(pOutputPath)) {
				std::filesystem::create_directories(pOutputPath);
			}

			// Get everything after the last backslash
			const std::string trailingFileName = fileName.substr(fileName.find_last_of("\\") + 1, fileName.length());
			const std::string outputPath = std::string(pOutputPath) + "\\" + trailingFileName;


			FILE* pFile = fopen(outputPath.c_str(), "wb");
			if (pFile) {
				fwrite(pStart, 1, pFileData->size, pFile);
				fclose(pFile);
			}

			inIndex = inIndex + 1;
			printf("\n");
		} while (inIndex < pFileHeader->fileCount);
	}
}

int main(int argc, char** argv)
{
	Initialize();
		
	argparse::ArgumentParser program("KyaBank");

	argparse::ArgumentParser listCommand("list");
	listCommand.add_description("List files contained in the .BNK file");
	listCommand.add_argument("file")
		.help("The path to a .BNK file")
		.required();

	argparse::ArgumentParser extractCommand("extract");
	extractCommand.add_description("Extract files contained in the .BNK file");
	extractCommand.add_argument("file")
		.help("The path to a .BNK file")
		.required();
	extractCommand.add_argument("-o", "--output")
		.help("The path to extract the files to")
		.required();

	program.add_subparser(listCommand);
	program.add_subparser(extractCommand);

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		std::exit(1);
	}

	if (program.is_subcommand_used(listCommand)) {
		ListFiles(listCommand.get<std::string>("file").c_str());
	}

	if (program.is_subcommand_used(extractCommand)) {
		ExtractFiles(extractCommand.get<std::string>("file").c_str(), extractCommand.get<std::string>("-o").c_str());
	}

	return 0;
}