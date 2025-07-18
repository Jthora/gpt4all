cmake_minimum_required(VERSION 3.16)
project(SimpleServerTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql)

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)

# Add the test executable
add_executable(test_simpleserver 
    ../test_simpleserver.cpp
    ../minimal_simpleserver.cpp
)

# Add the production server executable
add_executable(gpt4all_server
    ../gpt4all_server.cpp
    ../minimal_simpleserver.cpp
)

# Link Qt libraries
target_link_libraries(test_simpleserver 
    Qt6::Core 
    Qt6::Network
    Qt6::Sql
)

# Link Qt libraries for production server
target_link_libraries(gpt4all_server
    Qt6::Core 
    Qt6::Network
    Qt6::Sql
)

# Include directories
target_include_directories(test_simpleserver PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
