find_package(Qt6 REQUIRED COMPONENTS Test)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unit/CMakeLists.txt")
    add_subdirectory(unit)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/e2e/CMakeLists.txt")
    add_subdirectory(e2e)
endif()
