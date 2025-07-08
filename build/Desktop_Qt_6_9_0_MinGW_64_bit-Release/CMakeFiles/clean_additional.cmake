# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\Deskmon_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Deskmon_autogen.dir\\ParseCache.txt"
  "Deskmon_autogen"
  )
endif()
