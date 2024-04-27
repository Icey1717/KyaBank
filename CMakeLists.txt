cmake_minimum_required(VERSION 3.24.2)

project(KyaBank)

set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# These should be setup as proper link libraries in the future, but for now they are just git submodules
set(lib_src 
	"edBank/sources/edBank.cpp"
	"edBank/sources/edBankBuffer.cpp"
	"edBank/sources/edBankDebug.cpp"
	"edBank/sources/edBankFile.cpp"
	"edBank/sources/edBankFiler.cpp"
	"edBank/sources/edBankStackFile.cpp"

	"edBank/include/edBank.h"
	"edBank/include/edBankBuffer.h"
	"edBank/include/edBankDebug.h"
	"edBank/include/edBankFile.h"
	"edBank/include/edBankFiler.h"
	"edBank/include/edBankStackFile.h"

	"edFile/include/edFileFiler.h"
	"edFile/include/edFile.h"
	"edFile/include/edFilePath.h"
	"edFile/include/edFileCRC32.h"
	"edFile/include/ps2/_edFileFilerCDVD.h"

	"edFile/sources/edFileFiler.cpp"
	"edFile/sources/edFile.cpp"
	"edFile/sources/edFilePath.cpp"
	"edFile/sources/edFileCRC32.cpp"
	"edFile/sources/ps2/_edFileFilerCDVD.cpp"
)

set (app_src
	"include/Types.h"
	"include/edStr.h"
	"include/edMem.h"
	"include/edSystem.h"

	"src/main.cpp"
	"src/edStr.cpp"
	"src/edSystem.cpp"
)

add_executable(KyaBank 
	${lib_src}
	${app_src}
)

target_compile_definitions(KyaBank PRIVATE "PLATFORM_WIN")

target_include_directories(KyaBank PRIVATE "src" "include" "edBank/include" "edFile/include" "argparse/include")