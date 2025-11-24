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

# --- MUDANÇA AQUI: Adicionado resource.rc ---
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/BotCore.cpp
    src/ConfigManager.cpp
    src/DatabaseManager.cpp
    src/CommandHandler.cpp
    src/EventHandler.cpp
    src/ReportGenerator.cpp
    src/Utils.cpp
    src/TicketManager.cpp
    src/commands/VisitasCommands.cpp
    src/commands/LeadCommands.cpp
    src/commands/SolicitacaoCommands.cpp
    src/commands/PlacaCommands.cpp
    src/commands/CompraCommands.cpp
    src/commands/GerarPlanilhaCommand.cpp
    src/commands/TicketCommands.cpp
    src/commands/EstoqueCommands.cpp
    src/commands/TodoCommands.cpp
    resource.rc
)
# --------------------------------------------

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