# minimum CMake version required

cmake_minimum_required(VERSION 3.15)

# Project name, version and description

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

# Add DPP as dependency

add_subdirectory(libs/DPP)




set(ZLIB_DIR ${PROJECT_SOURCE_DIR}/libs/zlib-1.3.1)

set(XLSXWRITER_DIR ${PROJECT_SOURCE_DIR}/libs/libxlsxwriter)




set(ZLIB_LIB ${ZLIB_DIR}/build/Release/zlib.lib)

set(XLSXWRITER_LIB ${XLSXWRITER_DIR}/build/Release/xlsxwriter.lib)



if(NOT EXISTS ${ZLIB_LIB})

    message(FATAL_ERROR "Zlib library not found at ${ZLIB_LIB}. Did you build it in Release mode inside libs/zlib-1.3.1?")

endif()

if(NOT EXISTS ${XLSXWRITER_LIB})

    message(FATAL_ERROR "Libxlsxwriter library not found at ${XLSXWRITER_LIB}. Did you build it in Release mode inside libs/libxlsxwriter?")

endif()





set(ZLIB_INCLUDE_DIR ${ZLIB_DIR} ${ZLIB_DIR}/build) # Pasta raiz e pasta de build da zlib

set(XLSXWRITER_INCLUDE_DIR ${XLSXWRITER_DIR}/include) # Pasta include da libxlsxwriter



# Create an executable

add_executable(${PROJECT_NAME}

    src/main.cpp


)



# Linking libraries

target_link_libraries(${PROJECT_NAME} PRIVATE

    dpp               

    ${ZLIB_LIB}       

    ${XLSXWRITER_LIB} 

)



# Specify includes

target_include_directories(${PROJECT_NAME} PRIVATE

    libs/DPP/include          

    include                   

    ${ZLIB_INCLUDE_DIR}       

    ${XLSXWRITER_INCLUDE_DIR} 

)



# Set C++ version

set_target_properties(${PROJECT_NAME} PROPERTIES

    CXX_STANDARD 20

    CXX_STANDARD_REQUIRED ON

)
