cmake_minimum_required(VERSION 3.5)
project(motor_test LANGUAGES CXX)


# # Default to C99
# if(NOT CMAKE_C_STANDARD)
#   set(CMAKE_C_STANDARD 99)
# endif()

# Default to C++14
# if(NOT CMAKE_CXX_STANDARD)
  # set(CMAKE_CXX_STANDARD 17)
  # set(CMAKE_CXX_STANDARD_REQUIRED ON)
# endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-pedantic)
  # 익명구조체 사용하면 안됨(C++ -> -Wno-pedantic 무시하기)
endif()


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


#set(CMAKE_C_FLAGS "$/usr/xenomai/bin/xeno-config --posix --cflags")
execute_process(COMMAND /usr/xenomai/bin/xeno-config --posix --cflags
		)
set(CMAKE_STATIC_LINKER_FLAGS "$/usr/xenomai/bin/xeno-config --posix --ldflags")

INCLUDE_DIRECTORIES(
  /usr/xenomai/include 
  /usr/xenomai/include/cobalt 
          /usr/xenomai/include/rtdm 
					/usr/xenomai/include/xenomai /{Project folder} 
          /usr/SOEM/soem 
          /usr/SOEM/oshw/linux
          /usr/SOEM/osal/linux
          /usr/SOEM/osal
          /usr/sdk_826_linux_3.3.13/middleware)

LINK_DIRECTORIES(/usr/xenomai/lib 
                /usr/xenomai/lib/xenomai
                /usr/SOEM/build 
                /usr/sdk_826_linux_3.3.13/middleware)

LINK_LIBRARIES(analogy cobalt copperplate -L/usr/xenomai/lib/xenomai/bootstrap.o rt soem 826_64)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(motor_commu REQUIRED)

set(PROJECT_SOURCES 
  #src/main.cpp 
  src/ecat_func.cpp 
  #src/s826DAQ.cpp
  # src/serial_linux.cpp
  src/plant_shm.cpp
  )
include_directories(include)

# add_executable(pub ${PROJECT_SOURCES})
# ament_target_dependencies(pub rclcpp std_msgs motor_commu)
# target_include_directories(pub PUBLIC ${CMAKE_SOURCE_DIR}/include)
# target_link_libraries(pub pthread)


add_executable(pub_shm ${PROJECT_SOURCES})
ament_target_dependencies(pub_shm rclcpp std_msgs motor_commu)
target_include_directories(pub_shm PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(pub_shm pthread)


# add_executable(sub src/ctrl_node.cpp)
# ament_target_dependencies(sub rclcpp std_msgs motor_commu)
# target_include_directories(sub PUBLIC ${CMAKE_SOURCE_DIR}/include)
# target_link_libraries(sub pthread)



install(TARGETS
  pub_shm
    # sub
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
