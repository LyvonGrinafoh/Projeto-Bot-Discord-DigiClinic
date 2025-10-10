# minimum CMake version required
cmake_minimum_required(VERSION 3.15)
# Project name, version and description
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

execute_process( COMMAND git submodule update --init --recursive )

# Add DPP as dependency
add_subdirectory(libs/DPP)

# Create an executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/tabela.cpp
    # your others files...
 "include/tabela.h" "src/tabela.cpp")

# Linking libraries
target_link_libraries(${PROJECT_NAME}
    dpp
)

# Specify includes
target_include_directories(${PROJECT_NAME} PRIVATE
    libs/DPP/include
    include
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20 # or 20 if you want something more recent
    CXX_STANDARD_REQUIRED ON
)
