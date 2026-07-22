cmake_minimum_required(VERSION 3.20)
project(GraphSandbox VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(glm REQUIRED)

add_executable(graph-sandbox 
main.cpp
vendor/stb_impl.cpp
vendor/tinygltf_impl.cpp
vendor/tiny_obj_impl.cpp
)

target_link_libraries( graph-sandbox
glm::glm
)

target_include_directories(graph-sandbox
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)