cmake_minimum_required(VERSION 3.20)

project(my_c_project VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_executable(my_app
    main.c
)
find_package(bfutils REQUIRED)
target_link_libraries(my_app PRIVATE bfutils::bfutils)
