cmake_minimum_required(VERSION 2.6)

#needed libraries
find_package(Qt4 4.4.3 COMPONENTS QtCore QtGui)

# for windows point to our project, then it should
# make a debug release
set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# globally add inlude directory
include_directories(include)

#add any necessary source files
add_subdirectory(src)
# add_subdirectory(isql++)
add_subdirectory(qtsql++)
# add_subdirectory(tests)
