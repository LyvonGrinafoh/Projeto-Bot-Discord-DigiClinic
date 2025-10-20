# minimum CMake version required

cmake_minimum_required(VERSION 3.15)

# Project name, version and description

project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")



# --- Início do Bloco ZLIB ---

# Adiciona o módulo para poder baixar coisas

include(FetchContent)



# Declara de onde baixar o ZLIB

FetchContent_Declare(

  zlib

  GIT_REPOSITORY https://github.com/madler/zlib.git

  GIT_TAG        v1.3.1 # Usando uma versão estável

)



# Manda o CMake baixar e configurar o zlib

FetchContent_MakeAvailable(zlib)

# --- Fim do Bloco ZLIB ---

execute_process( COMMAND git submodule update --init --recursive )



set(BUILD_SHARED_LIBS ON)

# Add DPP as dependency

add_subdirectory(libs/DPP)



# --- Define variáveis para os caminhos das libs (ajuste se necessário) ---

# Garanta que essas pastas existem dentro de 'libs/'

set(ZLIB_DIR ${PROJECT_SOURCE_DIR}/libs/zlib-1.3.1)

set(XLSXWRITER_DIR ${PROJECT_SOURCE_DIR}/libs/libxlsxwriter)



# --- Encontre os arquivos .lib (Ajuste Release/Debug conforme necessário) ---

# Se você compilou em Debug, troque Release por Debug aqui

set(ZLIB_LIB ${ZLIB_DIR}/build/Release/zlib.lib)

set(XLSXWRITER_LIB ${XLSXWRITER_DIR}/build/Release/xlsxwriter.lib)



# --- Verifique se os arquivos .lib existem ---

# Se um desses der erro, significa que a compilação da lib correspondente falhou

# ou o arquivo .lib não está no caminho esperado (build/Release/).

if(NOT EXISTS ${ZLIB_LIB})

    message(FATAL_ERROR "Zlib library not found at ${ZLIB_LIB}. Did you build it in Release mode inside libs/zlib-1.3.1?")

endif()

if(NOT EXISTS ${XLSXWRITER_LIB})

    message(FATAL_ERROR "Libxlsxwriter library not found at ${XLSXWRITER_LIB}. Did you build it in Release mode inside libs/libxlsxwriter?")

endif()



# --- Define variáveis para os includes ---

# Garanta que os .h estão nessas pastas

set(ZLIB_INCLUDE_DIR ${ZLIB_DIR} ${ZLIB_DIR}/build) # Pasta raiz e pasta de build da zlib

set(XLSXWRITER_INCLUDE_DIR ${XLSXWRITER_DIR}/include) # Pasta include da libxlsxwriter



# Create an executable

add_executable(${PROJECT_NAME}

    src/main.cpp

    # src/tabela.cpp # Adicione de volta se você tiver esse arquivo

)



# Linking libraries

target_link_libraries(${PROJECT_NAME} PRIVATE

    dpp               # Linka a DPP (já definida pelo add_subdirectory)

    ${ZLIB_LIB}       # Linka o arquivo .lib específico da zlib

    ${XLSXWRITER_LIB} # Linka o arquivo .lib específico da libxlsxwriter

)



# Specify includes

target_include_directories(${PROJECT_NAME} PRIVATE

    libs/DPP/include          # Headers da DPP

    include                   # Seus próprios headers (se tiver a pasta 'include')

    ${ZLIB_INCLUDE_DIR}       # Headers da Zlib

    ${XLSXWRITER_INCLUDE_DIR} # Headers da libxlsxwriter

)



# Set C++ version

set_target_properties(${PROJECT_NAME} PROPERTIES

    CXX_STANDARD 20

    CXX_STANDARD_REQUIRED ON

)