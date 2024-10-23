cmake_minimum_required(VERSION 3.5)
project(ros2_xeno)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_C_FLAGS "$/usr/xenomai/bin/xeno-config --posix --cflags")
execute_process(COMMAND /usr/xenomai/bin/xeno-config --posix --cflags
		)
set(CMAKE_STATIC_LINKER_FLAGS "$/usr/xenomai/bin/xeno-config --posix --ldflags")

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)



INCLUDE_DIRECTORIES(/usr/xenomai/include /usr/xenomai/include/cobalt /usr/xenomai/include/rtdm 
										/usr/xenomai/include/xenomai /{Project folder})
										LINK_DIRECTORIES(/usr/xenomai/lib /usr/xenomai/lib/xenomai)
LINK_LIBRARIES(analogy cobalt copperplate -L/usr/xenomai/lib/xenomai/bootstrap.o rt)

add_executable(xeno_node src/pub_shm.cpp)
ament_target_dependencies(xeno_node rclcpp std_msgs)
target_link_libraries(xeno_node pthread)

add_executable(xeno_node2 src/subs_shm.cpp)
ament_target_dependencies(xeno_node2 rclcpp std_msgs)
target_link_libraries(xeno_node2 pthread)


add_executable(xeno_node_rcl src2/pub_rcl.cpp)
ament_target_dependencies(xeno_node_rcl rclcpp std_msgs)
target_link_libraries(xeno_node_rcl pthread)

add_executable(xeno_node_rcl2 src2/subs_rcl.cpp)
ament_target_dependencies(xeno_node_rcl2 rclcpp std_msgs)
target_link_libraries(xeno_node_rcl2 pthread)

install(TARGETS
    xeno_node
	xeno_node2
    xeno_node_rcl
    xeno_node_rcl2
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch 
    DESTINATION share/${PROJECT_NAME})


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
