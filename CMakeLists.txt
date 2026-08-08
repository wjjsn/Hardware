cmake_minimum_required(VERSION 3.22)
enable_language(CXX)

add_library(Hardware STATIC
    "${CMAKE_CURRENT_LIST_DIR}/OLED/font.cpp"
)
target_include_directories(Hardware PUBLIC "${CMAKE_CURRENT_LIST_DIR}")