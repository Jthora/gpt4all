cmake_minimum_required(VERSION 3.16)
project(RealGPT4AllAPIServerMicroHTTPD)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components (standard ones)
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql)

# Find libmicrohttpd
find_package(PkgConfig REQUIRED)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)

# Enable automatic MOC
set(CMAKE_AUTOMOC ON)

# Add GPT4All sources (only the essential ones)
set(GPT4ALL_SOURCES
    gpt4all-chat/src/chat.cpp
    gpt4all-chat/src/chatllm.cpp
    gpt4all-chat/src/chatmodel.cpp
    gpt4all-chat/src/database.cpp
    gpt4all-chat/src/modellist.cpp
    gpt4all-chat/src/mysettings.cpp
)

# Add executable
add_executable(real_gpt4all_server_microhttpd 
    real_gpt4all_server_microhttpd.cpp
    ${GPT4ALL_SOURCES}
)

# Include directories
target_include_directories(real_gpt4all_server_microhttpd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gpt4all-chat/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gpt4all-backend/include
    ${MICROHTTPD_INCLUDE_DIRS}
)

# Link GPT4All backend library
target_link_libraries(real_gpt4all_server_microhttpd 
    Qt6::Core 
    Qt6::Network
    Qt6::Sql
    ${MICROHTTPD_LIBRARIES}
    llmodel
)

# Link directories for GPT4All backend
target_link_directories(real_gpt4all_server_microhttpd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/build/bin
    ${CMAKE_CURRENT_SOURCE_DIR}/gpt4all-backend/build
)
