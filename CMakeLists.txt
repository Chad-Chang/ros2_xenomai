cmake_minimum_required(VERSION 3.5)
project(mujoco_example
    VERSION 0.1
    DESCRIPTION "예제 프로젝트"
    LANGUAGES CXX
    )


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


#set(CMAKE_C_FLAGS "$/usr/xenomai/bin/xeno-config --posix --cflags")
execute_process(COMMAND /usr/xenomai/bin/xeno-config --posix --cflags
		)
set(CMAKE_STATIC_LINKER_FLAGS "$/usr/xenomai/bin/xeno-config --posix --ldflags")



INCLUDE_DIRECTORIES(/usr/xenomai/include /usr/xenomai/include/cobalt /usr/xenomai/include/rtdm 
										/usr/xenomai/include/xenomai /{Project folder})
										LINK_DIRECTORIES(/usr/xenomai/lib /usr/xenomai/lib/xenomai)
LINK_LIBRARIES(analogy cobalt copperplate -L/usr/xenomai/lib/xenomai/bootstrap.o rt)




# find dependencies
find_package(ament_cmake REQUIRED)
find_package(Eigen3 REQUIRED)
link_directories(${Eigen_INCLUDE_DIRS})
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(motor_commu REQUIRED)



set (SOURCES
  myproject/src/MuJoCoMessageHandler.cpp
  myproject/src/main.cc
  
)
include_directories(include)


add_executable (simul ${SOURCES})
ament_target_dependencies(simul rclcpp std_msgs motor_commu)
target_include_directories(simul PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/myproject/include
  /usr/include/eigen3
)

target_link_directories(simul PUBLIC       #Library directory
    ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(simul
    pthread
    mujoco
    glfw
    dl
)




install(TARGETS
  simul
    # sub
  DESTINATION lib/${PROJECT_NAME})



# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

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
