cmake_minimum_required(VERSION 3.1)

project(xen_test VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "$/usr/xenomai/bin/xeno-config --posix --cflags")
set(CMAKE_STATIC_LINKER_FLAGS "$/usr/xenomai/bin/xeno-config --posix --ldflags")

INCLUDE_DIRECTORIES(/usr/xenomai/include /usr/xenomai/include/cobalt /usr/xenomai/include/rtdm 
										/usr/xenomai/include/xenomai /{Project folder})
LINK_DIRECTORIES(/usr/xenomai/lib /usr/xenomai/lib/xenomai)
LINK_LIBRARIES(analogy cobalt copperplate -L/usr/xenomai/lib/xenomai/bootstrap.o rt)


set(PROJECT_SOURCES
        main.cpp
)
add_executable(main ${PROJECT_SOURCES})

target_link_libraries(main pthread)