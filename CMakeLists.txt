cmake_minimum_required(VERSION 3.5)
project(SimpleServerTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

qt6_standard_project_setup()

add_executable(test_simpleserver 
    test_simpleserver.cpp
    gpt4all-chat/src/simpleserver.cpp
)

target_link_libraries(test_simpleserver Qt6::Core Qt6::Widgets Qt6::Network)

target_include_directories(test_simpleserver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
