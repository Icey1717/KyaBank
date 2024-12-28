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

set (pizza_src
	"pizza-png/src/Chunk.cpp"
	"pizza-png/src/Chunk.h"
	"pizza-png/src/Image.cpp"
	"pizza-png/src/Image.h"
	"pizza-png/src/Pixel.cpp"
	"pizza-png/src/Pixel.h"
)

add_library(Pizza STATIC
	${pizza_src}
)

set (app_src
	"include/Types.h"
	"include/edStr.h"
	"include/edMem.h"
	"include/edSystem.h"
	"include/ed3D.h"
	"include/PointerConv.h"

	"src/main.cpp"
	"src/ed3D.cpp"
	"src/PointerConv.cpp"
	"src/ConvertTexture.cpp"
	"src/ConvertTexture.h"
	"src/edStr.cpp"
	"src/edSystem.cpp"
)

add_executable(KyaBank 
	${lib_src}
	${app_src}
)

include_directories("include", "src")

# Set the Standalone variable for the texture project
set(Standalone ON CACHE BOOL "Enable standalone mode" FORCE)
add_subdirectory("KyaTexture")

add_subdirectory("TextureUpload")

target_link_libraries(KyaBank PRIVATE Texture TextureUpload Pizza)

target_compile_definitions(KyaBank PRIVATE "PLATFORM_WIN")

target_include_directories(KyaBank PRIVATE "src" "include" "edBank/include" "edFile/include" "argparse/include", "${CMAKE_CURRENT_LIST_DIR}")