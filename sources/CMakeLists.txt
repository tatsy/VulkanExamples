include_directories(${CMAKE_CURRENT_LIST_DIR}/ext/stb)
include_directories(${CMAKE_CURRENT_LIST_DIR}/ext/tinyobjloader)

file(GLOB SUBDIR_LIST RELATIVE ${CMAKE_CURRENT_LIST_DIR} "*")
foreach(SUBDIR ${SUBDIR_LIST})
  if (NOT ${SUBDIR} STREQUAL "ext")
    if (IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/${SUBDIR}")
      BUILD_EXAMPLE(${SUBDIR})
    endif()
  endif()
endforeach()
