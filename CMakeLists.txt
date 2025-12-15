cmake_minimum_required(VERSION 3.15)
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

include(FetchContent)

FetchContent_Declare(
  zlib
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG        v1.3.1
)

FetchContent_MakeAvailable(zlib)

execute_process( COMMAND git submodule update --init --recursive )

set(BUILD_SHARED_LIBS ON)
add_subdirectory(libs/DPP)

set(XLSXWRITER_DIR ${PROJECT_SOURCE_DIR}/libs/libxlsxwriter)
set(XLSXWRITER_LIB ${XLSXWRITER_DIR}/build/Release/xlsxwriter.lib)
set(XLSXWRITER_INCLUDE_DIR ${XLSXWRITER_DIR}/include)

if(NOT EXISTS ${XLSXWRITER_LIB})
    message(FATAL_ERROR "Libxlsxwriter library not found at ${XLSXWRITER_LIB}. Did you build it in Release mode inside libs/libxlsxwriter?")
endif()

if(MSVC)
    add_compile_options(/utf-8 /EHsc)
endif()

# --- MUDANÇA: Usar GLOB para encontrar arquivos automaticamente ---
# Isso pega TODOS os .cpp dentro de src/ e src/commands/
file(GLOB SOURCES 
    "src/*.cpp" 
    "src/commands/*.cpp"
)

# Adiciona resource.rc se existir (se der erro, remova esta linha)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resource.rc")
    list(APPEND SOURCES resource.rc)
endif()

add_executable(${PROJECT_NAME} ${SOURCES} "include/httplib.h")
# ---------------------------------------------------------------

target_link_libraries(${PROJECT_NAME} PRIVATE
    dpp
    zlib
    ${XLSXWRITER_LIB}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    libs/DPP/include
    ${XLSXWRITER_INCLUDE_DIR}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE XLSXWRITER_DLL)
endif()