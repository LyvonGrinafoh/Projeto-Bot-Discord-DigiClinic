# minimum CMake version required
cmake_minimum_required(VERSION 3.15)
# Project name, version and description
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

execute_process( COMMAND git submodule update --init --recursive )

set(BUILD_SHARED_LIBS ON)
# Add DPP as dependency
add_subdirectory(libs/DPP)

# Create an executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/tabela.cpp
)

# Linking libraries
# AQUI A GENTE ADICIONA AS NOVAS LIBS, MANTENDO A DPP INTOCADA
target_link_libraries(${PROJECT_NAME} PRIVATE
    dpp

    # Caminhos completos para os arquivos .lib que compilamos
    "C:/Users/mathe/Documents/Bot Digital Clinic/zlib131/zlib-1.3.1/build/Release/zlib.lib"
    "C:/Users/mathe/Documents/Bot Digital Clinic/Projeto-Bot-Discord-DigiClinic/libxlsxwriter/build/Release/xlsxwriter.lib"
)

# Specify includes
# E AQUI A GENTE ADICIONA OS NOVOS HEADERS, MANTENDO OS DA DPP INTOCADOS
target_include_directories(${PROJECT_NAME} PRIVATE
    # Seus includes que já funcionavam
    libs/DPP/include
    include

    # Headers da ZLIB (um na raiz, outro na pasta de build)
    "C:/Users/mathe/Documents/Bot Digital Clinic/zlib131/zlib-1.3.1"
    "C:/Users/mathe/Documents/Bot Digital Clinic/zlib131/zlib-1.3.1/build"

    # Headers da libxlsxwriter (na pasta include do código-fonte)
    "C:/Users/mathe/Documents/Bot Digital Clinic/Projeto-Bot-Discord-DigiClinic/libxlsxwriter/include"
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20 # or 20 if you want something more recent
    CXX_STANDARD_REQUIRED ON
)