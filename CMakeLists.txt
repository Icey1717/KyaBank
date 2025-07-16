cmake_minimum_required(VERSION 3.19.4)

project(KyaBank)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
	add_compile_options("-fpermissive")
endif()

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

set( USE_PIZZA_PNG OFF CACHE BOOL "Use Pizza PNG" )

set(app_src
	"include/Types.h"
	"include/edStr.h"
	"include/edMem.h"
	"include/edSystem.h"
	"include/ed3D.h"
	"include/edList.h"
	"include/PointerConv.h"

	"src/main.cpp"
	"src/ed3D.cpp"
	"src/MeshBuffer.cpp"
	"src/PointerConv.cpp"
	"src/ConvertTexture.cpp"
	"src/ConvertTexture.h"
	"src/ConvertMesh.cpp"
	"src/ConvertMesh.h"
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

add_subdirectory("KyaMesh")

add_subdirectory("TextureUpload")

target_link_libraries(KyaBank PRIVATE Texture Mesh TextureUpload)

if(USE_PIZZA_PNG)
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

	target_link_libraries(KyaBank PRIVATE Pizza)

	target_compile_definitions(KyaBank PRIVATE "USE_PIZZA_PNG")
else()
  set (lodepng_src
	  "lodepng/lodepng.h"
	  "lodepng/lodepng.cpp"
  )

  add_library(lodepng STATIC
	  ${lodepng_src}
  )
  target_link_libraries(KyaBank PRIVATE lodepng)
  
  target_compile_definitions(KyaBank PRIVATE "USE_LODE_PNG")
endif()

target_compile_definitions(KyaBank PRIVATE "PLATFORM_WIN")

target_include_directories(KyaBank PRIVATE "src" "include" "edBank/include" "edFile/include" "argparse/include", "${CMAKE_CURRENT_LIST_DIR}")
