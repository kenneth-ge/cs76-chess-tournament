cmake_minimum_required (VERSION 2.8.11)
project (ChessAI)

add_executable(ChessAI "src/ChessAI.cpp")

add_subdirectory("${PROJECT_SOURCE_DIR}/libraries/include/chess" EXCLUDE_FROM_ALL)
include_directories(${THC_CHESS_INCLUDE})
target_link_libraries(ChessAI thc_chess_static) # or thc_chess for dynamic linking