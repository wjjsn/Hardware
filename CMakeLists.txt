cmake_minimum_required(VERSION 3.22)
enable_language(CXX)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_LIST_DIR}/OLED/font.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PN532/keys.cpp"
    )
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/PN532"
    )